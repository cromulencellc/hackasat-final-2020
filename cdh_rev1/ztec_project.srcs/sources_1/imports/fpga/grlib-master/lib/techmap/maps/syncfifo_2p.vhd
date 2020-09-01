------------------------------------------------------------------------------
--  This file is a part of the GRLIB VHDL IP LIBRARY
--  Copyright (C) 2003 - 2008, Gaisler Research
--  Copyright (C) 2008 - 2014, Aeroflex Gaisler
--  Copyright (C) 2015 - 2017, Cobham Gaisler
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 2 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program; if not, write to the Free Software
--  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
-----------------------------------------------------------------------------
-- Entity:        syncfifo_2p
-- File:          syncfifo_2p.vhd
-- Authors:       Pascal Trotta
--                Andrea Gianarro - Cobham Gaisler AB
-- Description:   Syncronous 2-port fifo with tech selection
-----------------------------------------------------------------------------
--  Notes: Generic fifo has the following features & limitations:
--         -almost full is driven only in write clock domain;
--         -almost empty is driven only in read clock domain;
--         -full and empty are driven in both clock domains;
--         -usedw is re-computed in each clock domain;
--         -in "first word fall through" mode rempty should be observed as data 
--          valid signal, as the first word written into the FIFO immediately
--          appears on the output. If renable is asserted while empty='0', and 
--          at the next read clock rising edge empty='1', then new read data is
--          not valid because fifo is empty. This does not apply in standard fifo
--          mode, i.e., when empty is asserted, the last read data is valid;
--         -it works also if rclk = wclk. With sepclk=0 synchronization stages
--          and gray encoder/decoder are not instantiated, since not necessary.
------------------------------------------------------------------------------

library ieee;
library techmap;
use ieee.std_logic_1164.all;
use techmap.gencomp.all;
use work.allmem.all;
library grlib;
use grlib.config.all;
use grlib.config_types.all;
use grlib.stdlib.all;

entity syncfifo_2p is
  generic (
    tech  : integer := 0;   -- target technology
    abits : integer := 10;  -- fifo address bits (actual fifo depth = 2**abits)
    dbits : integer := 32;  -- fifo data width
    sepclk : integer := 1;  -- 1 = asynchrounous read/write clocks, 0 = synchronous read/write clocks
    afullwl : integer := 0; -- almost full min writes left until full (0 makes equal to full)
    aemptyrl : integer := 0; -- almost empty min reads left until empty (0 makes equal to empty)
    fwft : integer := 0;     -- 1 = first word fall trough mode, 0 = standard mode
    piperead: integer := 0;   -- add full pipeline stage on read side (ping pong buffer)
    ft   : integer := 0;     -- enable EDAC on RAMs (GRLIB-FT only, passed on to syncram_2pft)
    custombits : integer := 1
    );
  port (
    rclk    : in std_logic;  -- read clock
    rrstn   : in std_logic;  -- read clock domain synchronous reset
    wrstn   : in std_logic;  -- write clock domain synchronous reset
    renable : in std_logic;  -- read enable
    rfull   : out std_logic; -- fifo full (synchronized in read clock domain)
    rempty  : out std_logic; -- fifo empty
    aempty  : out std_logic; -- fifo almost empty (depending on pempty threshold)
    rusedw  : out std_logic_vector(abits-1 downto 0);  -- fifo used words (synchronized in read clock domain)
    dataout : out std_logic_vector(dbits-1 downto 0);  -- fifo data output
    wclk    : in std_logic;  -- write clock
    write   : in std_logic;  -- write enable
    wfull   : out std_logic; -- fifo full
    afull   : out std_logic; -- fifo almost full (depending on pfull threshold)
    wempty  : out std_logic; -- fifo empty (synchronized in write clock domain)
    wusedw  : out std_logic_vector(abits-1 downto 0); -- fifo used words (synchronized in write clock domain)
    datain  : in std_logic_vector(dbits-1 downto 0); -- fifo data input
    testin  : in std_logic_vector(TESTIN_WIDTH-1 downto 0) := testin_none;
    error    : out std_logic_vector((((dbits+7)/8)-1)*(1-ft/4)+ft/4 downto 0); -- FT only
    errinj   : in std_logic_vector((((dbits + 7)/8)*2-1)*(1-ft/4)+(6*(ft/4)) downto 0) := (others => '0') -- FT only
    );
end;

architecture rtl of syncfifo_2p is

  component generic_fifo
    generic (tech  : integer := 0; abits : integer := 10; dbits : integer := 32;
    sepclk : integer := 1; afullwl : integer := 0; aemptyrl : integer := 0; fwft : integer := 0;
    ft : integer := 0; custombits : integer := 1);
    port (
      rclk    : in std_logic;
      rrstn   : in std_logic;
      wrstn   : in std_logic;
      renable : in std_logic;
      rfull   : out std_logic;
      rempty  : out std_logic;
      aempty  : out std_logic;
      rusedw  : out std_logic_vector(abits-1 downto 0);
      dataout : out std_logic_vector(dbits-1 downto 0);
      wclk    : in std_logic;
      write   : in std_logic;
      wfull   : out std_logic;
      afull   : out std_logic;
      wempty  : out std_logic;
      wusedw  : out std_logic_vector(abits-1 downto 0);
      datain  : in std_logic_vector(dbits-1 downto 0);
      testin  : in std_logic_vector(TESTIN_WIDTH-1 downto 0) := testin_none;
      error    : out std_logic_vector((((dbits+7)/8)-1)*(1-ft/4)+ft/4 downto 0);
      errinj   : in std_logic_vector((((dbits + 7)/8)*2-1)*(1-ft/4)+(6*(ft/4)) downto 0) := (others => '0')
      );
  end component;

  signal dataout_i: std_logic_vector(dbits-1 downto 0);
  signal rempty_i, renable_i: std_logic;
  signal error_i: std_logic_vector(error'high downto 0);

  type intarr is array(natural range <>) of integer;
  constant fwfttab: intarr(0 to 2) := (0,1,1);
  constant fwftx: integer := fwfttab(fwft+piperead);

  signal piperead_d0, piperead_d1: std_logic_vector(dbits-1 downto 0);
  signal piperead_e0, piperead_e1: std_logic_vector(error'high downto 0);
  signal piperead_valid, piperead_read: std_logic_vector(1 downto 0);
  signal piperead_selout: std_ulogic;

begin

-- Altera fifo
  alt : if ((tech = altera) or (tech = stratix1) or (tech = stratix2) or
            (tech = stratix3) or (tech = stratix4) or (tech = stratix5)) and ft=0 generate
    x0 : altera_fifo_dp generic map (tech, abits, dbits, sepclk, afullwl, aemptyrl, fwftx)
      port map (rclk, renable_i, rfull, rempty_i, aempty, rusedw, dataout_i, wclk,
        write, wfull, afull, wempty, wusedw, datain);
  end generate;

-- generic FIFO implemented using syncram_2p component
  inf : if ((tech /= altera) and (tech /= stratix1) and (tech /= stratix2) and 
            (tech /= stratix3) and (tech /= stratix4) and (tech /= stratix5)) or ft/=0 generate
    x0: generic_fifo generic map (tech, abits, dbits, sepclk, afullwl, aemptyrl, fwftx, ft, custombits)
      port map (rclk, rrstn, wrstn, renable_i, rfull, rempty_i, aempty, rusedw, dataout_i,
        wclk, write, wfull, afull, wempty, wusedw, datain, testin, error_i, errinj
                );
  end generate;

  nopout: if piperead=0 generate
    dataout <= dataout_i;
    rempty <= rempty_i;
    renable_i <= renable;
    error <= error_i;
    piperead_d0 <= (others => '0');
    piperead_d1 <= (others => '0');
    piperead_valid <= "00";
    piperead_selout <= '0';
    piperead_read <= "00";
  end generate;

  pout: if piperead=1 generate
    doregen: if fwft=0 generate
      dop: process(rclk)
      begin
        if rising_edge(rclk) then
          if piperead_selout='0' then
            dataout <= piperead_d0;
            error <= piperead_e0;
          else
            dataout <= piperead_d1;
            error <= piperead_e1;
          end if;
        end if;
      end process;
    end generate;
    nooreg: if fwft/=0 generate
      dataout <= piperead_d0 when piperead_selout='0' else piperead_d1;
      error <= piperead_e0 when piperead_selout='0' else piperead_e1;
    end generate;
    rempty <= not piperead_valid(0) and not piperead_valid(1);
    piperead_read(0) <= (not piperead_selout or piperead_valid(1)) and not piperead_valid(0) and not rempty_i;
    piperead_read(1) <= (piperead_selout     or piperead_valid(0)) and not piperead_valid(1) and not rempty_i;
    renable_i <= piperead_read(0) or piperead_read(1);
    pipeproc: process(rclk)
    begin
      if rising_edge(rclk) then
        piperead_valid(0) <= (piperead_valid(0) or piperead_read(0)) and not (renable and not piperead_selout);
        piperead_valid(1) <= (piperead_valid(1) or piperead_read(1)) and not (renable and piperead_selout);
        piperead_selout <= piperead_selout xor renable;
        if piperead_read(0)='1' then
          piperead_d0 <= dataout_i;
          piperead_e0 <= error_i;
        end if;
        if piperead_read(1)='1' then
          piperead_d1 <= dataout_i;
          piperead_e1 <= error_i;
        end if;
        if rrstn='0' then
          piperead_valid <= "00";
          piperead_selout <= '0';
        end if;
      end if;
    end process;
  end generate;

-- pragma translate_off
  nofifo : if (has_2pfifo(tech) = 0) and (has_2pram(tech) = 0) generate
    x : process
    begin
      assert false report "syncfifo_2p: technology " & tech_table(tech) &
	" not supported"
      severity failure;
      wait;
    end process;
  end generate;
  dmsg : if GRLIB_CONFIG_ARRAY(grlib_debug_level) >= 2 generate
    x : process
    begin
      assert false report "syncfifo_2p: " & tost(2**abits) & "x" & tost(dbits) &
       " (" & tech_table(tech) & ")"
      severity note;
      wait;
    end process;
  end generate;
-- pragma translate_on

end;


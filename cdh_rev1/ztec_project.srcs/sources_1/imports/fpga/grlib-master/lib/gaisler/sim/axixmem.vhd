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
-------------------------------------------------------------------------------
-- Entity:      aximem
-- File:        aximem.vhd
-- Author:      Magnus Hjorth, Cobham Gaisler AB
-- Modified     Alen Bardizbanyan, Cobham Gaisler AB
-- Description: Simulation AXI3/4 slave with SREC loader
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library grlib;
use grlib.amba.all;
use grlib.stdlib.all;

library gaisler;
use gaisler.sim.all;

entity axixmem is
  generic (
    fname: string;
    axibits: integer := AXIDW;
    rstmode: integer range 0 to 1
    );
  port (
    clk  : in std_ulogic;
    rst  : in std_ulogic;
    axisi: in axix_mosi_type;
    axiso: out axi_somi_type;
    conf_in : in aximem_conf_type := aximem_conf_type_def
  );
end;

architecture sim of axixmem is

  constant simCTO: time := 1 ns;
  constant nerror : integer := 100;
  signal axiso_int: axi_somi_type;

  constant axi_somi_idle : axi_somi_type := (
    aw => (ready => '0'),
    w => (ready => '0'),
    b => (id => "0000", resp => "00", valid => '0'),
    ar => (ready => '0'),
    r => (id => "0000", data => (others => '0'), resp => "00", last => '0', valid => '0')
    );

  signal rbin: ramback_in_array(1 to 1) := (others => ramback_in_none);
  signal rbout: ramback_out_array(1 to 1);

  signal rac_wait_for_valid : std_logic := '0';
  signal wac_wait_for_valid : std_logic := '0';
  signal wdc_wait_for_valid : std_logic := '0';
  
  type rwq_entry is record
    valid: boolean;
    id: std_logic_vector(3 downto 0);
    addr: std_logic_vector(31 downto 0);
    len: std_logic_vector(7 downto 0);
    size: std_logic_vector(2 downto 0);
    error : std_logic_vector(1 downto 0);
    done: boolean; -- write queue only
  end record;
  type rwq_type is array(natural range <>) of rwq_entry;

  type wdataq_entry is record
    valid: boolean;
    id: std_logic_vector(3 downto 0);
    data: std_logic_vector(axibits-1 downto 0);
    strb: std_logic_vector(axibits/8-1 downto 0);
  end record;
  type wdataq_type is array(natural range <>) of wdataq_entry;

  type error_addr_type is array (0 to nerror-1) of std_logic_vector(31 downto 0);
  type error_type is record
    enabled : std_logic;
    dstype : std_logic_vector(nerror-1 downto 0);  --0->slv_error,1->dec_error
    valid : std_logic_vector(nerror-1 downto 0);
    addr : error_addr_type;
    mask : error_addr_type;   
  end record;

  constant error_reset : error_type := (
    enabled => '0',
    dstype => (others=>'0'),
    valid => (others=>'0'),
    addr => (others=>(others=>'0')),
    mask => (others=>(others=>'0'))
    );

  function bool10(b: boolean) return std_ulogic is
  begin
    if b then return '1'; else return '0'; end if;
  end bool10;

  signal rqdbg: rwq_type(0 to 3);
  signal err_arr : error_type := error_reset;

begin

  axiso <= axiso_int after simCTO;

  rb: ramback
    generic map (
      abits => 32-log2(axibits/8),
      dbits => axibits,
      fname => fname,
      autoload => 1,
      nports => 1,
      endian => 1,
      rstmode => rstmode)
    port map (
      bein => rbin,
      beout => rbout
      );


  p: process
    variable o,po: axi_somi_type := axi_somi_idle;
    variable rq, wq: rwq_type(0 to 3);
    variable wdq: wdataq_type(0 to 7);
    variable i : integer;
    variable vaddr: std_logic_vector(31 downto 0);
    variable vwr : std_logic_vector(15 downto 0);
    variable err_match : std_logic;

  begin
    wait until rising_edge(clk);
    po := o;
    o.r.resp := "00";
   
    --------------------------------------------------------------------------
    -- Read handling
    --------------------------------------------------------------------------
    -- Enqueue on read address queue
    if po.ar.ready='1' and axisi.ar.valid='1' then
      i := 0;
      while rq(i).valid loop i:=i+1; end loop;
      rq(i).valid := true;
      rq(i).id := axisi.ar.id;
      rq(i).addr := axisi.ar.addr;
      rq(i).len := axisi.ar.len;
      rq(i).size := axisi.ar.size;
    end if;
    -- Advance read queue
    if po.r.valid='1' and axisi.r.ready='1' then
      assert rq(0).valid;
      if po.r.last='1' then
        assert rq(0).len = "00000000";
        rq(0 to rq'high-1) := rq(1 to rq'high);
        rq(rq'high).valid := false;
      else
        assert rq(0).len /= "00000000";
        rq(0).addr := std_logic_vector(unsigned(rq(0).addr)+2**(to_integer(unsigned(rq(0).size))));
        rq(0).len := std_logic_vector(unsigned(rq(0).len)-1);
      end if;
    end if;
    if rac_wait_for_valid = '0' then
      o.ar.ready := bool10(not rq(rq'high).valid);
    else
      o.ar.ready := '0';
      if (bool10(not rq(rq'high).valid) = '1') and axisi.ar.valid = '1' then
        o.ar.ready := '1';
      end if;
    end if;

    o.r.valid  := bool10(rq(0).valid);
    o.r.id := rq(0).id;
    o.r.last := bool10(rq(0).valid and rq(0).len="00000000");
    if rq(0).valid then
      -- Do read
      vaddr := (others => '0');
      vaddr(31-log2(axibits/8) downto 0) := rq(0).addr(31 downto log2(axibits/8));
      rbin(1).addr <= vaddr;
      if rbout(1).addr /= vaddr then
        wait until rbout(1).addr=vaddr;
      end if;
      o.r.data(axibits-1 downto 0) := rbout(1).dout(axibits-1 downto 0);
     
      if err_arr.enabled = '1' then
        for i in 0 to nerror-1 loop
          if err_arr.valid(i) = '1' then
            err_match := '1';
            for j in 0 to 31 loop
              if (rq(0).addr(j) /= err_arr.addr(i)(j)) and (err_arr.mask(i)(j) = '0') then
                err_match := '0';
                exit;
              end if;          
            end loop;

            if err_match = '1' then
              if err_arr.dstype(i) = '0' then
                o.r.resp := "10";
              else
                o.r.resp := "11";
              end if;
              exit;
            end if;
             
          end if;
        end loop;
      end if;
      
    end if;

    --------------------------------------------------------------------------
    -- Write handling
    --------------------------------------------------------------------------
    -- Retire write addr queue
    if po.b.valid='1' and axisi.b.ready='1' then
      assert wq(0).valid and wq(0).done and wq(0).id=po.b.id;
      wq(0 to wq'high-1) := wq(1 to wq'high);
      wq(wq'high).valid := false;
    end if;
    -- Enqueue on write address queue
    if po.aw.ready='1' and axisi.aw.valid='1' then
      i := 0;
      while wq(i).valid loop i:=i+1; end loop;
      wq(i).valid := true;
      wq(i).done := false;
      wq(i).id := axisi.aw.id;
      wq(i).addr := axisi.aw.addr;
      wq(i).len := axisi.aw.len;
      wq(i).size := axisi.aw.size;
      wq(i).error := "00";
    end if;
    -- Enqueue on write data queue
    if po.w.ready='1' and axisi.w.valid='1' then
      i := 0;
      while wdq(i).valid loop i:=i+1; end loop;
      wdq(i).valid := true;
      wdq(i).id := axisi.w.id;
      wdq(i).data := axisi.w.data;
      wdq(i).strb := axisi.w.strb;
    end if;
    -- Advance write data
    if wdq(0).valid then
      i := 0;
      while wq(i).valid and wq(i).id /= wdq(0).id loop i:=i+1; end loop;
      if wq(i).valid then
        assert not wq(i).done;
        vaddr := (others => '0');
        vaddr(31-log2(axibits/8) downto 0) := wq(i).addr(31 downto log2(axibits/8));
        vwr := (others => '0');
        vwr(axibits/8-1 downto 0) := wdq(0).strb;
        rbin(1).addr <= vaddr;
        rbin(1).wr <= vwr;
        rbin(1).din(axibits-1 downto 0) <= wdq(0).data;
        if rbin(1).wr /= vwr then
          wait until rbin(1).wr = vwr;
        end if;
        rbin(1).wr <= (others => '0');
        if wq(i).len="00000000" then
          wq(i).done := true;
        end if;
        wq(i).addr := std_logic_vector(unsigned(wq(i).addr)+2**(to_integer(unsigned(wq(i).size))));
        wq(i).len := std_logic_vector(unsigned(wq(i).len)-1);
        wdq(0 to wdq'high-1) := wdq(1 to wdq'high);
        wdq(wdq'high).valid := false;

        if err_arr.enabled = '1' then
          for z in 0 to nerror-1 loop
            if err_arr.valid(z) = '1' then
              err_match := '1';
              for j in 0 to 31 loop
                if (wq(0).addr(j) /= err_arr.addr(z)(j)) and (err_arr.mask(z)(j) = '0') then
                  err_match := '0';
                  exit;
                end if;          
              end loop;

              if err_match = '1' then
                if err_arr.dstype(z) = '0' then
                  wq(0).error := "10";
                else
                  wq(0).error := "11";
                end if;
                exit;
              end if;
              
            end if;
          end loop;
        end if;
        
      end if;
    end if;
    if wac_wait_for_valid = '0' then
      o.aw.ready := bool10(not wq(wq'high).valid);
    else
      o.aw.ready := '0';
      if (bool10(not wq(wq'high).valid) = '1') and axisi.aw.valid = '1' then
        o.aw.ready := '1';
      end if;
    end if;

    if wdc_wait_for_valid = '0' then
      o.w.ready := bool10(not wdq(wdq'high).valid);
    else
      o.w.ready := '0';
      if (bool10(not wdq(wdq'high).valid) = '1') and axisi.w.valid = '1' then
        o.w.ready := '1';
      end if;
    end if;
    
    o.b.valid := bool10(wq(0).valid and wq(0).done);
    o.b.id := wq(0).id;

    o.b.resp := "00";
    if bool10(wq(0).valid and wq(0).done) = '1' then
      o.b.resp := wq(0).error;
    end if;

    axiso_int <= o;
    rqdbg <= rq;
  end process;



  err:process
   
    begin
      
      wait until conf_in.err.entry_strobe = '1';
      err_arr.enabled <= conf_in.err.enabled;
      err_arr.dstype(conf_in.err.id) <= conf_in.err.dstype;
      err_arr.valid(conf_in.err.id) <= conf_in.err.valid;
      err_arr.addr(conf_in.err.id) <= conf_in.err.addr;
      err_arr.mask(conf_in.err.id) <= conf_in.err.mask;
      wait until conf_in.err.entry_strobe = '0';
        
  end process;

  wac:process

    begin

      wait until conf_in.wac.entry_strobe = '1';
      wac_wait_for_valid <= conf_in.wac.wait_for_valid;
      wait until conf_in.wac.entry_strobe = '0';

    end process;

  wdc:process

    begin

      wait until conf_in.wdc.entry_strobe = '1';
      wdc_wait_for_valid <= conf_in.wdc.wait_for_valid;
      wait until conf_in.wdc.entry_strobe = '0';

  end process; 

  rac:process

    begin

      wait until conf_in.rac.entry_strobe = '1';
      rac_wait_for_valid <= conf_in.rac.wait_for_valid;
      wait until conf_in.rac.entry_strobe = '0';

  end process; 
    
end;

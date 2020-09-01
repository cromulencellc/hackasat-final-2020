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
--============================================================================--
-- Design unit  : GRVERSION Interface (Entity & architecture declarations)
--
-- File name    : grversion.vhd
--
-- Library      : {independent}
--
-- Authors      : Aeroflex Gaisler AB
--
-- Contact      : mailto:support@gaisler.com
--                http://www.gaisler.com
--
-- Disclaimer   : All information is provided "as is", there is no warranty that
--                the information is correct or suitable for any purpose,
--                neither implicit nor explicit.
--
--------------------------------------------------------------------------------
-- Version  Author   Date           Changes
--
-- 1.0      SH       10 Feb 2006    New design
--------------------------------------------------------------------------------
library  IEEE;
use      IEEE.Std_Logic_1164.all;

library  grlib;
use      grlib.amba.all;
use      grlib.stdlib.all;

library  gaisler;
use      gaisler.misc.all;

--pragma translate_off
use      Std.TextIO.all;
--pragma translate_on

entity grversion is
   generic (
      pindex:           Integer :=  0;
      paddr:            Integer :=  0;
      pmask:            Integer := 16#fff#;
      versionnr:        Integer := 16#0123#;
      revisionnr:       Integer := 16#4567#);
   port (
      rstn:       in    Std_ULogic;
      clk:        in    Std_ULogic;
      apbi:       in    APB_Slv_In_Type;
      apbo:       out   APB_Slv_Out_Type);
end entity grversion;

architecture rtl of grversion is

   -----------------------------------------------------------------------------
   -- addressing constants
   -----------------------------------------------------------------------------
   --                                                    765432
   constant cVerStat:   Std_Logic_Vector(7 downto 2) := "000000";

   -----------------------------------------------------------------------------
   -- configuration constants
   -----------------------------------------------------------------------------
   constant REVISION:   Integer := 0;

   constant pconfig:    apb_config_type := (
      0 => ahb_device_reg (1, 16#03A#, 0, REVISION, 0),
      1 => apb_iobar(paddr, pmask));

begin

   -----------------------------------------------------------------------------
   -- combinatorial logic
   -----------------------------------------------------------------------------
   comb: process(rstn, apbi)
      variable prdata:     Std_Logic_Vector(31 downto 0);
      variable paddr7_2:   Std_Logic_Vector(7 downto 2);
   begin
      paddr7_2 := apbi.paddr(7 downto 2);

      -- read registers
      prdata := (others => '0');
      case paddr7_2  is
         when cVerStat =>
            prdata(31 downto 16) := Conv_Std_Logic_Vector(versionnr, 16);
            prdata(15 downto  0) := Conv_Std_Logic_Vector(revisionnr, 16);
         when others =>
            null;
      end case;

      -- variable to signal assigment
      apbo.prdata <= prdata;                             -- drive apb read bus
      apbo.pirq   <= (others => '0');
   end process;

   -----------------------------------------------------------------------------
   -- configuration assigment
   -----------------------------------------------------------------------------
   apbo.pindex    <= pindex;
   apbo.pconfig   <= pconfig;

   -----------------------------------------------------------------------------
   -- boot message
   -----------------------------------------------------------------------------
-- pragma translate_off
    bootmsg : report_version
      generic map(
         "grversion" & tost(pindex) & ": " &
         "General Version Number rev " & tost(REVISION));
-- pragma translate_on


end architecture rtl; --======================================================--


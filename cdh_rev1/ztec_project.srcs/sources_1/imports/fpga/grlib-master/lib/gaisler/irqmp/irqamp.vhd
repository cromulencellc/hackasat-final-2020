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
-- Entity: 	irqamp
-- File:	irqamp.vhd
-- Author:	Jiri Gaisler - Gaisler Research
-- Modified:    AMP extension: Jan Andersson - Aeroflex Gaisler
-- Contact:     support@gaisler.com
-- Description:	Multi-processor APB interrupt controller. Implements a
--		two-level interrupt controller for 15 interrupts.
--              Also has extended support for Asymmetric Multi-Processing
------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
library grlib;
use grlib.amba.all;
use grlib.stdlib.all;
use grlib.devices.all;
library gaisler;
use gaisler.leon3.all;

entity irqamp is
  generic (
    pindex     : integer := 0;
    paddr      : integer := 0;
    pmask      : integer := 16#fff#;
    ncpu       : integer := 1;
    eirq       : integer := 0;
    nctrl      : integer range 1 to 16 := 1;
    tstamp     : integer range 0 to 16 := 0;
    wdogen     : integer range 0 to 1 := 0;
    nwdog      : integer range 1 to 16 := 1;
    dynrstaddr : integer range 0 to 0 := 0;
    rstaddr    : integer range 0 to 16#fffff# := 0;
    extrun     : integer range 0 to 1 := 0;
    irqmap     : integer := 0;
    exttimer   : integer range 0 to 1 := 0;
    bootreg    : integer range 0 to 1 := 1
  );
  port (
    rst    : in  std_ulogic;
    clk    : in  std_ulogic;
    apbi   : in  apb_slv_in_type;
    apbo   : out apb_slv_out_type;
    irqi   : in  irq_out_vector(0 to ncpu-1);
    irqo   : out irq_in_vector(0 to ncpu-1);
    wdog   : in  std_logic_vector(nwdog-1 downto 0) := (others => '0');
    cpurun : in  std_logic_vector(ncpu-1 downto 0) := (others => '0');
    timer  : in  std_logic_vector(31 downto 0) := (others => '0');
    rstmap : in  std_logic_vector((64*5)-1 downto 0) := (others => '0')
  );
end;

architecture rtl of irqamp is

constant REVISION : integer := 4;

constant pconfig : apb_config_type := (
  0 => ahb_device_reg ( VENDOR_GAISLER, GAISLER_IRQMP, 0, REVISION, 0),
  1 => apb_iobar(paddr, pmask));

function IMAP_HIGH return integer is
begin
  if irqmap = 0 then
    return 0;
  elsif irqmap = 3 then
    return 63;
  elsif eirq /= 0 or irqmap = 2 then
    return 31;
  end if;
  return 15;
end function IMAP_HIGH;
constant IMAP_LOW : integer := 0; -- allow remap of irq line 0
function IMAP_LEN return integer is
begin
  if irqmap = 0 then
    return 1;
  elsif eirq /= 0 or irqmap = 3 then
    return 5;
  end if;
  return 4;
end function IMAP_LEN;

type mask_type is array (0 to ncpu-1) of std_logic_vector(15 downto 1);
type mask2_type is array (0 to ncpu-1) of std_logic_vector(15 downto 0);
type irl_type is array (0 to ncpu-1) of std_logic_vector(3 downto 0);
type irl2_type is array (0 to ncpu-1) of std_logic_vector(4 downto 0);
type ilevel_type is array(0 to nctrl-1) of std_logic_vector(15 downto 1); 
type ipend_type is array(0 to nctrl-1) of std_logic_vector(15 downto 1);
type ipend2_type is array(0 to nctrl-1) of std_logic_vector(15 downto 0);
type ibroadcast_type is array (0 to nctrl-1) of std_logic_vector(15 downto 1);
type irqmap_type is array (IMAP_LOW to (IMAP_HIGH)) of std_logic_vector(IMAP_LEN-1 downto 0);

type reg_type is record
  imask		: mask_type;
  ilevel	: ilevel_type;
  ipend		: ipend_type;
  iforce	: mask_type;
  ibroadcast	: ibroadcast_type;
  irl    	: irl_type;
  cpurst	: std_logic_vector(ncpu-1 downto 0);
  imap          : irqmap_type;
end record;

type ereg_type is record
  imask		: mask2_type;
  ipend		: ipend2_type;
  irl    	: irl2_type;
end record;

type icsel_type is array (0 to ncpu-1) of std_logic_vector(log2x(nctrl)-1 downto 0);
type icsel_int_type is array (0 to ncpu-1) of integer; 

type areg_type is record
  icf   : std_ulogic;
  lock  : std_ulogic;
  icsel : icsel_type;
end record;

constant TNUM : integer := log2x(2**tstamp);
constant SBITS : integer := 32;         -- Number of bits in stamp

type tsisel_type is array (0 to TNUM-1) of std_logic_vector(4 downto 0);

type tstamp_type is array (0 to TNUM-1) of std_logic_vector(SBITS-1 downto 0); 

type tsreg_type is record
  s1    : std_logic_vector(0 to TNUM-1);
  s2    : std_logic_vector(0 to TNUM-1);
  ks    : std_logic_vector(0 to TNUM-1);
  sel   : tsisel_type;
  stmp1 : tstamp_type;
  stmp2 : tstamp_type;
end record;

type tsreg_array_type is array (0 to nctrl-1) of tsreg_type;

type treg_type is record
  cnt   : std_logic_vector((SBITS-1)*(1-exttimer) downto 0);
  tsreg : tsreg_array_type;
end record;

type wirq_array_type is array (0 to nctrl-1) of std_logic_vector(3 downto 0);
type wmsk_array_type is array (0 to nctrl-1) of std_logic_vector(nwdog-1 downto 0);

type wreg_type is record
  irq : wirq_array_type;
  msk : wmsk_array_type;
end record;

type rstvec_array_type is array (0 to ncpu-1) of std_logic_vector(31 downto 12);

constant HRC  : integer := 7;          -- # of reset cycles
constant LERC : integer := 1;          -- extra rst clocks within processor

type rreg_type is record
  setaddr       : std_logic_vector(ncpu-1 downto 0);
  newaddr       : std_logic_vector(31 downto 2);
  setaddrboot   : std_ulogic;
  forceerr      : std_logic_vector(ncpu-1 downto 0);
  clkcount      : std_logic_vector(2 downto 0);
end record;

function prioritize(b : std_logic_vector(15 downto 0)) return std_logic_vector is
variable a : std_logic_vector(15 downto 0);
variable irl : std_logic_vector(3 downto 0);
variable level : integer range 0 to 15;
begin
  irl := "0000"; level := 0; a := b;
  for i in 15 downto 0 loop
    level := i;
    if a(i) = '1' then exit; end if;
  end loop;
  irl := conv_std_logic_vector(level, 4);
  return(irl);
end;

signal r, rin : reg_type;
signal r2, r2in : ereg_type;
signal r3, r3in : areg_type;
signal r4, r4in : treg_type;
signal r5, r5in : wreg_type;
signal r6, r6in : rreg_type;

begin

  comb : process(rst, r, r2, r3, r4, r5, r6, apbi, irqi, wdog, cpurun, timer)
  variable v       : reg_type;
  variable temp    : mask_type;
  variable prdata  : std_logic_vector(31 downto 0);
  variable tmpirq  : std_logic_vector(15 downto 0);
  variable tmpvar  : std_logic_vector(15 downto 1);
  variable cpurunx : std_logic_vector(ncpu-1 downto 0);
  variable v2      : ereg_type;
  variable irl2    : std_logic_vector(3 downto 0);
  variable ipend2  : std_logic_vector(ncpu-1 downto 0);
  variable temp2   : mask2_type;
  variable v3      : areg_type;
  variable ctrl    : icsel_int_type;
  variable apbcsel : integer range 0 to nctrl-1;
  variable v4      : treg_type;
  variable tinc    : std_ulogic;
  variable v5      : wreg_type;
  variable wa      : std_ulogic;
  variable v6      : rreg_type;
  variable hrdrst  : std_logic_vector(ncpu-1 downto 0);
  variable irq     : std_logic_vector(NAHBIRQ-1 downto 0);
  variable timeval : std_logic_vector(SBITS-1 downto 0);
  variable vcpu    : std_logic_vector(3 downto 0);
  variable bootreg_sel : std_ulogic;
  variable paddr   : std_logic_vector(19 downto 2);

  begin

    v := r; v.cpurst := (others => '0');
    if extrun = 0 then cpurunx := (others => '0'); cpurunx(0) := '1';
    else cpurunx := cpurun; end if;
    v2 := r2; v3 := r3; v4 := r4; v5 := r5; v6 := r6;
    tmpvar := (others => '0'); ipend2 := (others => '0');
    for i in 0 to ncpu-1 loop
      if nctrl = 1 then ctrl(i) := 0;
      else ctrl(i) := conv_integer(r3.icsel(i)); end if;
    end loop;
    if nctrl = 1 then apbcsel := 0;
    else apbcsel := conv_integer(apbi.paddr(12+log2x(nctrl)-1 downto 12)); end if;
    tinc := '0'; wa := '0'; hrdrst := (others => '0');
    timeval := (others => '0');

    paddr := apbi.paddr(19 downto 2);
    paddr(19 downto 8) := paddr(19 downto 8) and not std_logic_vector(to_unsigned(pmask,12));

-- prioritize interrupts
  
    if eirq /= 0 then
      for i in 0 to ncpu-1 loop
        temp2(i) := r2.ipend(ctrl(i)) and r2.imask(i);
        ipend2(i) := orv(temp2(i));
      end loop;
    end if;

    for i in 0 to ncpu-1 loop
      temp(i) := ((r.iforce(i) or r.ipend(ctrl(i))) and r.imask(i));
      if eirq /= 0 then temp(i)(eirq) := temp(i)(eirq) or ipend2(i); end if;
      v.irl(i) := prioritize((temp(i) and r.ilevel(ctrl(i))) & '0');
      if v.irl(i) = "0000" then
        if eirq /= 0 then temp(i)(eirq) := temp(i)(eirq) or ipend2(i); end if;
        v.irl(i) := prioritize((temp(i) and not r.ilevel(ctrl(i))) & '0');
      end if;
    end loop;

    if bootreg /= 0 then
      if r6.clkcount/="000" then
        v6.clkcount := std_logic_vector(unsigned(r6.clkcount)-1);
      end if;
    end if;

-- register read

    prdata := (others => '0');
    case apbi.paddr(7 downto 5) is
    when "000" =>
      case apbi.paddr(4 downto 2) is
      when "000" => prdata(15 downto 1) := r.ilevel(apbcsel);
      when "001" => 
	prdata(15 downto 1) := r.ipend(apbcsel);
        if eirq /= 0 then prdata(31 downto 16) := r2.ipend(apbcsel); end if;
      when "010" => prdata(15 downto 1) := r.iforce(0);
      when "011" =>
      when "100" | "101" =>
        prdata(31 downto 28) := conv_std_logic_vector(ncpu-1, 4);
        prdata(19 downto 16) := conv_std_logic_vector(eirq, 4);
        for i in 0 to ncpu -1 loop prdata(i) := irqi(i).pwd; end loop;
        if ncpu > 1 then
          prdata(27) := '1';
          case apbi.paddr(4 downto 2) is
            when "101" =>
              prdata := (others => '0');
              prdata(15 downto 1) := r.ibroadcast(apbcsel);
            when others =>  
          end case;
        end if;
        if apbcsel=0 then
          prdata(27) := '1';
          if bootreg /= 0 then prdata(26) := '1'; end if;
        end if;
      when "110" =>
        if apbcsel=0 then
          for i in 0 to ncpu-1 loop prdata(i):=irqi(i).err; end loop;
        end if;
      when "111" =>
        if wdogen /= 0 then
          prdata(31 downto 27) := conv_std_logic_vector(nwdog, 5);
          prdata(19 downto 16) := r5.irq(apbcsel);
          prdata(nwdog-1 downto 0) := r5.msk(apbcsel);
        end if;
      when others =>
      end case;
    when "001" =>
      if nctrl /= 1 then
        case apbi.paddr(4 downto 2) is
        when "000" =>                -- 0x20
          prdata(31 downto 28) := conv_std_logic_vector(nctrl-1, 4);
          prdata(1) := r3.icf;
          prdata(0) := r3.lock;
        when "001" =>                -- 0x24
          for i in 0 to ncpu-1 loop
            prdata(28+log2x(nctrl)-1-4*i downto 28-4*i) := r3.icsel(i);
          end loop;
        when "010" =>                -- 0x28
          if ncpu > 8 then
            for i in 8 to ncpu-1 loop
               prdata(28+log2x(nctrl)-1-4*(i-8) downto 28-4*(i-8)) := r3.icsel(i);
            end loop;
          end if;
        when others => null;
        end case;
      end if;
    when "010" | "011" =>
      for i in 0 to ncpu-1 loop
	if i = conv_integer( apbi.paddr(5 downto 2)) then
	  prdata(15 downto 1) := r.imask(i);
          if eirq /= 0 then prdata(31 downto 16) := r2.imask(i); end if;
	end if;
      end loop;
    when "100" | "101" =>
      for i in 0 to ncpu-1 loop
	if i = conv_integer( apbi.paddr(5 downto 2)) then
	    prdata(15 downto  1) := r.iforce(i);
	end if;
      end loop;
    when "110" | "111" =>
      if eirq /= 0 then
        for i in 0 to ncpu-1 loop
	  if i = conv_integer( apbi.paddr(5 downto 2)) then
	    prdata(4 downto 0) := r2.irl(i);
	  end if;
        end loop;
      end if;
    when others =>
    end case;

-- register write

    if (((apbi.psel(pindex) and apbi.penable and apbi.pwrite) = '1') and
        (tstamp = 0 or paddr(8) = '0') and (bootreg = 0 or paddr(9) = '0')) then
      case apbi.paddr(7 downto 5) is
      when "000" =>
        case apbi.paddr(4 downto 2) is
        when "000" => v.ilevel(apbcsel) := apbi.pwdata(15 downto 1);
        when "001" => v.ipend(apbcsel)  := apbi.pwdata(15 downto 1);
          if eirq /= 0 then v2.ipend(apbcsel) := apbi.pwdata(31 downto 16); end if;
        when "010" =>
          if nctrl = 1 or apbcsel = ctrl(0) then v.iforce(0) := apbi.pwdata(15 downto 1); end if;
        when "011" => v.ipend(apbcsel)  := r.ipend(apbcsel) and not apbi.pwdata(15 downto 1);
          if eirq /= 0 then v2.ipend(apbcsel) := r2.ipend(apbcsel) and not apbi.pwdata(31 downto 16); end if;
        when "100" =>
          for i in 0 to ncpu -1 loop
            if (apbcsel = ctrl(i)) then 
              v.cpurst(i) := apbi.pwdata(i);
            end if;
          end loop;
        when "110" =>
          if bootreg /= 0 then
            if apbcsel=0 then
              v6.forceerr := v6.forceerr or apbi.pwdata(ncpu-1 downto 0);
              v6.clkcount := "111";
            end if;
          end if;
        when "111" =>
          if wdogen /= 0 then
            v5.irq(apbcsel) := apbi.pwdata(19 downto 16);
            v5.msk(apbcsel) := apbi.pwdata(nwdog-1 downto 0);
          end if;
        when others =>
          if ncpu > 1 then
            case apbi.paddr(4 downto 2) is
              when "101" =>
                v.ibroadcast(apbcsel) := apbi.pwdata(15 downto 1);
              when others =>  
            end case;
          end if;
        end case;
      when "001" =>
        if nctrl /= 1 then
          case apbi.paddr(4 downto 2) is
          when "000" =>                -- 0x20
            v3.icf  := apbi.pwdata(1);
            v3.lock := apbi.pwdata(0);
          when "001" =>                -- 0x24
            if r3.lock = '0' then
              for i in 0 to ncpu-1 loop
                v3.icsel(i) := apbi.pwdata(28+log2x(nctrl)-1-4*i downto 28-4*i);
              end loop;
            end if;
          when "010" =>                -- 0x28
          if ncpu > 8 then
            if r3.lock = '0' then
              for i in 8 to ncpu-1 loop
                v3.icsel(i) := apbi.pwdata(28+log2x(nctrl)-1-4*(i-8) downto 28-4*(i-8));
              end loop;
            end if;
          end if;
          when others => null;
          end case;
        end if;
      when "010" | "011" =>
        for i in 0 to ncpu-1 loop
	  if (i = conv_integer( apbi.paddr(5 downto 2))) and (apbcsel = ctrl(i)) then
            v.imask(i) := apbi.pwdata(15 downto 1);
            if eirq /= 0 then v2.imask(i) := apbi.pwdata(31 downto 16); end if;
	  end if;
        end loop;
      when "100" | "101" =>
        for i in 0 to ncpu-1 loop
	  if (i = conv_integer( apbi.paddr(5 downto 2))) then
            if (nctrl = 1 or apbcsel = ctrl(i) or r3.icf = '1') then
              v.iforce(i) := r.iforce(i) or apbi.pwdata(15 downto 1);
            end if;
            if (nctrl = 1 or apbcsel = ctrl(i)) then
              v.iforce(i) := v.iforce(i) and not apbi.pwdata(31 downto 17);
            end if;
	  end if;
        end loop;
      when others =>
      end case;
    end if;

-- implement processor reboot / monitor regs
    vcpu := apbi.paddr(5 downto 2);
    bootreg_sel := '0';

    if bootreg /= 0 then
      if r6.clkcount="000" then
        if orv(r6.setaddr)='1' then
          if r6.newaddr(2)='0' then
            v6.newaddr(2):='1';
          else
            if r6.setaddrboot='1' then
              v.cpurst := v.cpurst or r6.setaddr;
            end if;
            v6.setaddr := (others => '0');
          end if;
        end if;
        for i in 0 to ncpu-1 loop
          v6.forceerr(i) := v6.forceerr(i) and not irqi(i).err;
        end loop;
      end if;
      -- Alias bootregs into 256B space if ncpu <= 8
      if paddr(9 downto 6)="1000" then bootreg_sel:='1'; end if;
      if ncpu <= 8 and paddr(9 downto 6)="0001" and apbi.paddr(5)='1' then
        bootreg_sel := '1';
      end if;
      if apbcsel /= 0 then bootreg_sel:='0'; end if;
      if ncpu <= 8 then vcpu(3):='0'; end if;
      if (apbi.psel(pindex) and apbi.penable)='1' and bootreg_sel='1' then
        -- Reg read
        prdata := (others => '0');
        -- Reg write
        if apbi.pwrite='1' then
          for i in 0 to ncpu-1 loop
            if i = conv_integer( vcpu) then
              v6.setaddr(i) := '1';
              v6.setaddrboot := apbi.pwdata(0);
            end if;
          end loop;
          v6.newaddr := apbi.pwdata(31 downto 3) & "0";
          v6.clkcount := "111";
        end if;                         -- pwrite
      end if;                           -- psel/paddr
    end if;                             -- bootreg/=0

-- optionally remap interrupts

    irq := (others => '0');
    if irqmap /= 0 then
      if ((apbi.psel(pindex) and apbi.penable and andv(paddr(9 downto 8))) = '1'
          and apbcsel = 0) then
        prdata := (others => '0');
        for i in r.imap'range loop
          if i/4 = conv_integer(apbi.paddr(5 downto 2)) then
            prdata(IMAP_LEN-1+(24-(i mod 4)*8) downto (24-(i mod 4)*8)) := r.imap(i);
            if apbi.pwrite = '1' then
              v.imap(i) := apbi.pwdata(IMAP_LEN-1+(24-(i mod 4)*8) downto (24-(i mod 4)*8));
            end if;
          end if;
        end loop;
      end if;

      for i in 0 to IMAP_HIGH loop
        if i > NAHBIRQ-1 then
          exit;
        end if;
        if apbi.pirq(i) = '1' then
          irq(conv_integer(r.imap(i))) := '1';
        end if;
      end loop;
    else
      irq := apbi.pirq;
      v.imap := (others => (others => '0'));
    end if;

-- interrupt timestamping

    if tstamp /= 0 then
      -- Counter
      if exttimer = 0 then
        -- One possible extension here is to listen to dsuo.tstop and stop the
        -- timer when the LEON/DSU goes into debug. The same behaviour can be
        -- accomplished by using the external timer and stopping it in debug
        -- mode.
        for i in 0 to nctrl-1 loop
          for j in 0 to TNUM-1 loop
            tinc := tinc or orv(r4.tsreg(i).sel(j));
          end loop;
        end loop;
        if tinc = '1' then
          v4.cnt := r4.cnt + 1;
        end if;
        timeval := r4.cnt;
      else
        v4.cnt := (others => '0');
        timeval := timer(SBITS-1 downto 0);
      end if;

      
      -- Interrupt timestamp registers
      if (((apbi.psel(pindex) and apbi.penable and paddr(8)) = '1') and
          ((bootreg+irqmap) = 0 or paddr(9) = '0')) then  
        prdata := (others => '0');
        for i in 0 to TNUM-1 loop
          if i = conv_integer(apbi.paddr(7 downto 4)) then
            case apbi.paddr(3 downto 2) is
              when "00" => prdata(SBITS-1 downto 0) := timeval;
              when "01" =>
                prdata(31 downto 27) := conv_std_logic_vector(tstamp, 5);
                prdata(26) := r4.tsreg(apbcsel).s1(i);
                prdata(25) := r4.tsreg(apbcsel).s2(i);
                prdata(5) := r4.tsreg(apbcsel).ks(i);
                prdata(4 downto 0) := r4.tsreg(apbcsel).sel(i);
                if apbi.pwrite = '1' then
                  if apbi.pwdata(26) = '1' then v4.tsreg(apbcsel).s1(i) := '0'; end if;
                  if apbi.pwdata(25) = '1' then v4.tsreg(apbcsel).s2(i) := '0'; end if;
                  v4.tsreg(apbcsel).ks(i) := apbi.pwdata(5);
                  v4.tsreg(apbcsel).sel(i) := apbi.pwdata(4 downto 0);
                end if;
              when "10" => prdata := r4.tsreg(apbcsel).stmp1(i);
              when others => prdata := r4.tsreg(apbcsel).stmp2(i);
            end case;
          end if;
        end loop;
      end if;

      -- Timestamp assertions
      for i in 1 to 31 loop
        if i > NAHBIRQ-1 then
          exit;
        end if;
        if i > 15 and eirq = 0 then
          exit;
        end if;
        for j in 0 to nctrl-1 loop
          for k in 0 to TNUM-1 loop
            if i = conv_integer(r4.tsreg(j).sel(k)) then
              if (irq(i) and (r4.tsreg(j).ks(k) nand r4.tsreg(j).s1(k))) = '1' then
                v4.tsreg(j).stmp1(k) := timeval;
                v4.tsreg(j).s1(k) := '1';
                if r4.tsreg(j).ks(k) = '0' then v4.tsreg(j).s2(k) := '0'; end if; 
              end if;
            end if;
          end loop;
        end loop;
      end loop;

      -- Timestamp interrupt acknowledge
      for i in 0 to ncpu-1 loop
        for j in 0 to TNUM-1 loop
          if ((r4.tsreg(ctrl(i)).sel(j)(4) = '0' and r4.tsreg(ctrl(i)).sel(j)(3 downto 0) = irqi(i).irl) or
              (eirq /= 0 and r4.tsreg(ctrl(i)).sel(j)(4) = '1' and eirq = conv_integer(irqi(i).irl) and
               prioritize(temp2(i)) = r4.tsreg(ctrl(i)).sel(j)(3 downto 0))) then
            if (irqi(i).intack and not r4.tsreg(ctrl(i)).s2(j)) = '1' then
              v4.tsreg(ctrl(i)).stmp2(j) := timeval;
              v4.tsreg(ctrl(i)).s2(j) := '1';
            end if;
          end if;
        end loop;
      end loop;
    end if;

-- watchdog interrupts
    if wdogen /= 0 then
      for i in 0 to nctrl-1 loop
        wa := '0';
        for j in 0 to nwdog-1 loop
          wa := wa or (r5.msk(i)(j) and wdog(j));
        end loop;
        for j in 1 to 15 loop
          if j > NAHBIRQ-1 then
            exit;
          end if;
          if j = conv_integer(r5.irq(i)) then
            v.ipend(i)(j) := v.ipend(i)(j) or wa;
          end if;
        end loop;
      end loop;
    end if;
    
-- register new interrupts
    
    for i in 1 to 15 loop
      if i > NAHBIRQ-1 then
         exit;
      end if;
      if ncpu = 1 then
        for j in 0 to nctrl-1 loop
          v.ipend(j)(i) := v.ipend(j)(i) or irq(i);
        end loop;
      else
        for j in 0 to nctrl-1 loop
          v.ipend(j)(i) := v.ipend(j)(i) or (irq(i) and not r.ibroadcast(j)(i));
        end loop;
        for j in 0 to ncpu-1 loop
          tmpvar := v.iforce(j);
          tmpvar(i) := tmpvar(i) or (irq(i) and r.ibroadcast(ctrl(j))(i));
          v.iforce(j) := tmpvar;
        end loop;
      end if;
    end loop;

    if eirq /= 0 then
      for h in 0 to nctrl-1 loop
        for i in 16 to 31 loop
          if i > NAHBIRQ-1 then exit; end if;
          v2.ipend(h)(i-16) := v2.ipend(h)(i-16) or irq(i);
        end loop;
      end loop;
    end if;

-- interrupt acknowledge

    for i in 0 to ncpu-1 loop
      if irqi(i).intack = '1' then
        tmpirq := decode(irqi(i).irl);
        temp(i) := tmpirq(15 downto 1);
        v.iforce(i) := v.iforce(i) and not temp(i);
        v.ipend(ctrl(i))  := v.ipend(ctrl(i)) and not ((not r.iforce(i)) and temp(i));
        if eirq /= 0 then
          if eirq = conv_integer(irqi(i).irl) then
            v2.irl(i) := orv(temp2(i)) & prioritize(temp2(i));
            if v2.irl(i)(4) = '1' then
                v2.ipend(ctrl(i))(conv_integer(v2.irl(i)(3 downto 0))) := '0';
            end if;
 	  end if;
	end if;
      end if;
    end loop;

-- reset

    if rst = '0' then
      v.imask := (others => (others => '0'));
      v.iforce := (others => (others => '0'));
      v.ipend := (others => (others => '0'));
      if ncpu > 1 then
        v.ibroadcast := (others => (others => '0'));
      end if;
      if irqmap /= 0 then
        for i in r.imap'range loop
          if irqmap = 3 then
            v.imap(i) := rstmap(IMAP_LEN*i + (IMAP_LEN-1) downto IMAP_LEN*i);
          else
            v.imap(i) := conv_std_logic_vector(i, IMAP_LEN);
          end if;
        end loop;
      end if;
      v2.ipend := (others => (others => '0'));
      v2.imask := (others => (others => '0'));
      v2.irl := (others => (others => '0'));
      v3.icf := '0';
      v3.lock := '0';
      v3.icsel := (others => (others => '0'));
      if tstamp /= 0 then
        v4.cnt := (others => '0');
        for i in 0 to (nctrl-1) loop
          v4.tsreg(i).s1 := (others => '0');
          v4.tsreg(i).s2 := (others => '0');
          v4.tsreg(i).ks := (others => '0');
          v4.tsreg(i).sel := (others => (others => '0'));
          v4.tsreg(i).stmp1 := (others => (others => '0'));
          v4.tsreg(i).stmp2 := (others => (others => '0'));
        end loop;
      end if;
      if wdogen /= 0 then
        for i in 0 to (nctrl-1) loop
          v5.msk(i) := (others => '0');
        end loop;
      end if;
      if bootreg /= 0 then
        v6.forceerr := (others => '0');
        v6.setaddr := (others => '0');
      end if;
    end if;

    apbo.prdata <= prdata;
    for i in 0 to ncpu-1 loop
      irqo(i).irl <= r.irl(i); irqo(i).resume <= r.cpurst(i);
      if bootreg /= 0 then
        irqo(i).forceerr <= r6.forceerr(i);
        irqo(i).pwdsetaddr <= r6.setaddr(i);
        irqo(i).pwdnewaddr <= r6.newaddr;
        irqo(i).svtclrtt <= r6.setaddr(i);
      end if;
      irqo(i).rstrun <= cpurunx(i);
      irqo(i).rstvec <= (others => '0');
      irqo(i).index <= conv_std_logic_vector(i, 4);
    end loop;

    rin <= v; r2in <= v2; r3in <= v3; r4in <= v4; r5in <= v5; r6in <= v6;
    
  end process;

  apbo.pirq <= (others => '0');
  apbo.pconfig <= pconfig;
  apbo.pindex <= pindex;

  regs : process(clk)
  begin if rising_edge(clk) then r <= rin; end if; end process;
  
  dor2regs : if eirq /= 0 generate
    regs : process(clk)
    begin if rising_edge(clk) then r2 <= r2in; end if; end process;
  end generate;
  nor2regs : if eirq = 0 generate
--    r2 <= ((others => "0000000000000000"), "0000000000000000", (others => "00000"));
    driveregs1: for i in 0 to (nctrl-1) generate
      r2.ipend(i) <= (others => '0');
    end generate driveregs1;  
    driveregs2: for i in 0 to (ncpu-1) generate
      r2.imask(i) <= (others => '0');
      r2.irl(i) <= (others => '0');
    end generate driveregs2;  
  end generate;

  dor3regs : if nctrl /= 1 generate
    regs : process(clk)
    begin if rising_edge(clk) then r3 <= r3in; end if; end process;
  end generate;
  nor3regs : if nctrl = 1 generate
    r3.icf  <= '0';
    r3.lock <= '0';
    driveregs: for i in 0 to (ncpu-1) generate
      r3.icsel(i) <= (others => '0');
    end generate driveregs;  
  end generate;

  dor4regs : if tstamp /= 0 generate
    regs : process(clk)
    begin if rising_edge(clk) then r4 <= r4in; end if; end process;
  end generate;
  nor4regs : if tstamp = 0 generate
    r4.cnt <= (others => '0');
    driveregs: for i in 0 to (nctrl-1) generate
      r4.tsreg(i).s1 <= (others => '0');
      r4.tsreg(i).s2 <= (others => '0');
      r4.tsreg(i).ks <= (others => '0');
      r4.tsreg(i).sel <= (others => (others => '0'));
      r4.tsreg(i).stmp1 <= (others => (others => '0'));
      r4.tsreg(i).stmp2 <= (others => (others => '0'));
    end generate driveregs;  
  end generate;

  dor5regs : if wdogen /= 0 generate
    regs : process(clk)
    begin if rising_edge(clk) then r5 <= r5in; end if; end process;
  end generate;
  nor5regs : if wdogen = 0 generate
    driveregs: for i in 0 to (nctrl-1) generate
      r5.irq(i) <= (others => '0');
      r5.msk(i) <= (others => '0');
    end generate driveregs;  
  end generate;

  dor6regs : if bootreg /= 0 generate
    regs : process(clk)
    begin if rising_edge(clk) then r6 <= r6in; end if; end process;
  end generate;
  nor6regs : if bootreg = 0 generate
    r6.setaddr <= (others => '0');
    r6.newaddr <= (others => '0');
    r6.setaddrboot <= '0';
    r6.forceerr <= (others => '0');
    r6.clkcount <= (others => '0');
  end generate;
  
-- pragma translate_off
    bootmsg : report_version
    generic map ("irq(a)mp" &
	": Multi-processor Interrupt Controller rev " & tost(REVISION) &
	", #cpu " & tost(NCPU) & ", eirq " & tost(eirq) &
        ", nctrl " & tost(nctrl) & ", tstamp " & tost(tstamp) &
        ", wdog " & tost(wdogen*nwdog));
-- pragma translate_on

-- pragma translate_off
  cproc : process
  begin
    assert (irqmap = 0) or (apb_membar_size(pmask) >= 1024)
      report "IRQ(A)MP: irqmap /= 0 requires pmask to give memory area >= 1024 bytes"
      severity failure;
    wait;
  end process;
-- pragma translate_on

end;


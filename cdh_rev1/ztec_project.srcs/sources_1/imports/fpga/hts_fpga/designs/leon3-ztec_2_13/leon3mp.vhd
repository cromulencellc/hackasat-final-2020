------------------------------------------------------------------------------
--  LEON3 Demonstration design
--  Copyright (C) 2016 Cobham Gaisler
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
------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

library grlib;
use grlib.amba.all;
use grlib.stdlib.all;
use grlib.devices.all;

library techmap;
use techmap.gencomp.all;
use techmap.allclkgen.all;

library gaisler;
use gaisler.memctrl.all;
use gaisler.leon3.all;
use gaisler.uart.all;
use gaisler.misc.all;
use gaisler.spi.all;
use gaisler.i2c.all;
use gaisler.net.all;
use gaisler.jtag.all;
use gaisler.ddrpkg.all;

--pragma translate_off
use gaisler.sim.all;
library unisim;
use unisim.all;
--pragma translate_on

library esa;
use esa.memoryctrl.all;

use work.config.all;

entity leon3mp is
  generic (
    fabtech                 : integer := CFG_FABTECH;
    memtech                 : integer := CFG_MEMTECH;
    padtech                 : integer := CFG_PADTECH;
    clktech                 : integer := CFG_CLKTECH;
    disas                   : integer := CFG_DISAS;     -- Enable disassembly to console
    dbguart                 : integer := CFG_DUART;     -- Print UART on console
    pclow                   : integer := CFG_PCLOW;
    SIM_BYPASS_INIT_CAL     : string := "OFF";
    SIMULATION              : string := "FALSE";
    USE_MIG_INTERFACE_MODEL : boolean := false
    );
  port (
    sysclk             : in    std_ulogic;
    -- LEDs
    led                : out   std_logic_vector(7 downto 0);
    -- Buttons
    btnc               : in    std_ulogic;
    btnd               : in    std_ulogic;
    btnl               : in    std_ulogic;
    btnr               : in    std_ulogic;
    cpu_resetn         : in    std_ulogic;
    
    -- GPIO
    gpio_dout           : out  std_logic_vector(13 downto 0);
    gpio_din            : in   std_logic_vector(13 downto 0);
    gpio_oen            : out  std_logic_vector(13 downto 0);
    
    -- Switches
    sw                 : in    std_logic_vector(7 downto 0);    
    
    -- Debug UART
    dbg_uart_txd        : out    std_ulogic;
    dbg_uart_rxd        : in   std_ulogic;
    
    -- Debug JTAG
    dbg_jtag_tck        : in   std_ulogic;
    dbg_jtag_tms        : in   std_ulogic;
    dbg_jtag_tdi        : in   std_ulogic;
    dbg_jtag_tdo        : out  std_ulogic;
    
    -- CMD/TLM Interface -- Could be umbilical or radio depending on switch settings
    radio_txd          : out    std_ulogic;
    radio_rxd          : in     std_ulogic;
    
    -- RTEMs console
    console_txd        : out    std_ulogic;
    console_rxd        : in     std_ulogic;
    
    -- EyasSat C&DH Interface
    eyassat_txd        : out    std_ulogic;
    eyassat_rxd        : in     std_ulogic;
    
    -- Payload Interface
    payload_txd        : out    std_ulogic;
    payload_rxd        : in     std_ulogic;
    
    -- DDR3
    ddr3_dq           : inout std_logic_vector(15 downto 0);
    ddr3_dqs_p        : inout std_logic_vector(1 downto 0);
    ddr3_dqs_n        : inout std_logic_vector(1 downto 0);
    ddr3_addr         : out   std_logic_vector(13 downto 0);
    ddr3_ba           : out   std_logic_vector(2 downto 0);
    ddr3_ras_n        : out   std_logic;
    ddr3_cas_n        : out   std_logic;
    ddr3_we_n         : out   std_logic;
    ddr3_reset_n      : out   std_logic;
    ddr3_ck_p         : out   std_logic_vector(0 downto 0);
    ddr3_ck_n         : out   std_logic_vector(0 downto 0);
    ddr3_cke          : out   std_logic_vector(0 downto 0);
    ddr3_dm           : out   std_logic_vector(1 downto 0);
    ddr3_odt          : out   std_logic_vector(0 downto 0);
    
    -- Ethernet
    --eth_int_b          : in std_logic;
    --eth_mdc            : out std_logic;
    --eth_mdio           : inout std_logic;
    --eth_pme_b          : out std_ulogic;
    --eth_rst_b          : out std_logic;
    --eth_rxck           : in std_logic;
    --eth_rxctl          : in std_logic; 
    --eth_rxd            : in std_logic_vector(3 downto 0);
    --eth_txck           : in std_logic;
    --eth_txctl          : out std_logic; 
    --eth_txd            : out std_logic_vector(3 downto 0);
    --phy_txclk       : out   std_logic;
    --phy_txd         : out   std_logic_vector(3 downto 0);
    --phy_txctl_txen  : out   std_ulogic;
    --phy_rxd         : in    std_logic_vector(3 downto 0);
    --phy_rxctl_rxdv  : in    std_ulogic;
    --phy_rxclk       : in    std_ulogic;
    --phy_reset       : out   std_ulogic;
    --phy_mdio        : inout std_logic;
    --phy_mdc         : out   std_ulogic;
    --phy_int         : in    std_ulogic;  
    -- Fan PWM
    --fan_pwm            : out std_ulogic;    
    -- SPI
    spi_cs            : out   std_ulogic;
    spi_miso          : in   std_logic;
    spi_mosi          : out   std_logic;
    --spi_dq            : inout std_logic_vector(3 downto 0);
    spi_sck           : out   std_ulogic;
    
    spi0_mosi         : in  std_ulogic;
    spi0_miso         : out std_ulogic;
    spi0_sck          : in  std_ulogic;
    spi0_sel          : in  std_ulogic;
    
    i2c0_scl_o        : out std_ulogic;
    i2c0_scl_i        : in  std_ulogic;
    i2c0_scl_oen      : out  std_ulogic;
  	i2c0_sda_o        : out std_ulogic;
    i2c0_sda_i        : in  std_ulogic;
    i2c0_sda_oen      : out  std_ulogic

--    i2c0_scl          : inout std_ulogic;
--    i2c0_sda          : inout std_ulogic

    );
end;

architecture rtl of leon3mp is

--component IDELAYCTRL
--  port (
--     RDY : out std_ulogic;
--     REFCLK : in std_ulogic;
--     RST : in std_ulogic
--  );
--end component;

component IODELAYE1
  generic (
     DELAY_SRC : string := "I";
     IDELAY_TYPE : string := "DEFAULT";
     IDELAY_VALUE : integer := 0;
     ODELAY_VALUE : integer := 0
     
  );
  port (
     CNTVALUEOUT : out std_logic_vector(4 downto 0);
     DATAOUT     : out std_ulogic;
     C           : in std_ulogic;
     CE          : in std_ulogic;
     CINVCTRL    : in std_ulogic;
     CLKIN       : in std_ulogic;
     CNTVALUEIN  : in std_logic_vector(4 downto 0);
     DATAIN      : in std_ulogic;
     IDATAIN     : in std_ulogic;
     INC         : in std_ulogic;
     ODATAIN     : in std_ulogic;
     RST         : in std_ulogic;
     T           : in std_ulogic
  );
end component;


component ODELAYE2 
 generic (
     ODELAY_VALUE : integer := 0
  );
  port (
     C           : in std_ulogic;
     REGRST      : in std_ulogic;
     LD          : in std_ulogic;
     CE          : in std_ulogic;
     INC         : in std_ulogic;
     CINVCTRL    : in std_ulogic;
     CNTVALUEIN  : in std_logic_vector(4 downto 0);
     CLKIN       : in std_ulogic;
     ODATAIN     : in std_ulogic;
     LDPIPEEN    : in std_ulogic;
     DATAOUT     : out std_ulogic;
     CNTVALUEOUT : out std_logic_vector(4 downto 0)
  );
end component;

  signal vcc : std_logic;
  signal gnd : std_logic;

  signal gpioi : gpio_in_type;
  signal gpioo : gpio_out_type;

  signal apbi  : apb_slv_in_type;
  signal apbo  : apb_slv_out_vector := (others => apb_none);
  signal ahbsi : ahb_slv_in_type;
  signal ahbso : ahb_slv_out_vector := (others => ahbs_none);
  signal ahbmi : ahb_mst_in_type;
  signal ahbmo : ahb_mst_out_vector := (others => ahbm_none);

  signal cgi : clkgen_in_type;
  signal cgo, cgo1 : clkgen_out_type;
  signal cgi2   : clkgen_in_type;
  signal cgo2   : clkgen_out_type;

  signal u1i, dui : uart_in_type;
  signal u1o, duo : uart_out_type;
  
  signal u2i : uart_in_type;
  signal u2o : uart_out_type;
  
  signal u3i : uart_in_type;
  signal u3o : uart_out_type;

  signal u4i : uart_in_type;
  signal u4o : uart_out_type;
  
  signal irqi : irq_in_vector(0 to CFG_NCPU-1);
  signal irqo : irq_out_vector(0 to CFG_NCPU-1);

  signal dbgi : l3_debug_in_vector(0 to CFG_NCPU-1);
  signal dbgo : l3_debug_out_vector(0 to CFG_NCPU-1);

  signal dsui : dsu_in_type;
  signal dsuo : dsu_out_type;
  signal ndsuact : std_ulogic;

  signal ethi : eth_in_type;
  signal etho : eth_out_type;

  signal gpti : gptimer_in_type;

  signal spmi : spimctrl_in_type;
  signal spmo : spimctrl_out_type;
  
  signal spi0_spii : spi_in_type;
  signal spi0_spio : spi_out_type;

  signal i2c0_i2ci : i2c_in_type;
  signal i2c0_i2co : i2c_out_type;

  signal clkm : std_ulogic
  -- pragma translate_off 
  := '0'
  -- pragma translate_on
  ;
  
  signal clkm2x, rstn : std_ulogic;
  --signal tck, tms, tdi, tdo : std_ulogic;
  signal rstraw             : std_logic;
  signal lcpu_resetn        : std_logic;
  signal lock        : std_logic;
  signal clkinmig           : std_logic;

  signal clkref, calib_done, migrstn : std_logic;

  signal gmiii : eth_in_type;
  signal gmiio : eth_out_type;

  signal rgmiii,rgmiii_buf : eth_in_type;
  signal rgmiio : eth_out_type;

  signal ethernet_phy_int : std_logic;

  --signal ethi : eth_in_type;
  --signal etho : eth_out_type;
  signal gtx_clk,gtx_clk_nobuf,gtx_clk90 : std_ulogic;
  signal rstgtxn : std_logic;

  signal idelay_reset_cnt : std_logic_vector(3 downto 0);
  signal idelayctrl_reset : std_logic;
  signal io_ref           : std_logic;

  signal phy_txclk_delay : std_logic;

  attribute keep                     : boolean;
  attribute syn_keep                 : boolean;
  attribute syn_preserve             : boolean;
  attribute syn_keep of lock         : signal is true;
  attribute syn_keep of clkm         : signal is true;
  attribute syn_preserve of clkm     : signal is true;
  attribute keep of lock             : signal is true;
  attribute keep of clkm             : signal is true;

  constant BOARD_FREQ : integer := 48000;                                -- CLK input frequency in KHz
  constant CPU_FREQ   : integer := BOARD_FREQ * CFG_CLKMUL / CFG_CLKDIV;  -- cpu frequency in KHz
  constant I2C_FILTER : integer := (CPU_FREQ*5+50000)/100000+1;


begin

  vcc <= '1'; gnd <= '0';

----------------------------------------------------------------------
---  Reset and Clock generation  -------------------------------------
----------------------------------------------------------------------

  cgi.pllctrl <= "00";
  cgi.pllrst <= rstraw;

--  rst_pad : inpad generic map (tech => padtech, level => cmos, voltage => x15v)
--    port map (cpu_resetn, lcpu_resetn);
lcpu_resetn <= cpu_resetn;
  
  rst0 : rstgen
    port map (lcpu_resetn, clkm, lock, rstn, rstraw);
  lock <= calib_done when CFG_MIG_7SERIES = 1 else cgo.clklock;

  rst1 : rstgen         -- reset generator
  port map (lcpu_resetn, clkm, vcc, migrstn, open);

  -- clock generator
  clkgen_gen: if (CFG_MIG_7SERIES = 0) generate
    clkgen0 : clkgen
      generic map (fabtech, CFG_CLKMUL, CFG_CLKDIV, 0, 0, 0, 0, 0, BOARD_FREQ, 0)
      port map (sysclk, gnd, clkm, open, clkm2x, open, open, cgi, cgo, open, open, open);
  end generate;

  led(7) <= lcpu_resetn;
  led(6) <= calib_done;
  led(5) <= rstn;
  led(4) <= lock;                       -- Used in TB

  
----------------------------------------------------------------------
---  AHB CONTROLLER --------------------------------------------------
----------------------------------------------------------------------

  ahb0 : ahbctrl
    generic map (defmast => CFG_DEFMST, split => CFG_SPLIT,
                 rrobin  => CFG_RROBIN, ioaddr => CFG_AHBIO, ioen => 1,
                 nahbm => CFG_NCPU+CFG_AHB_UART+CFG_AHB_JTAG+CFG_GRETH,
                 nahbs => 9)
    port map (rstn, clkm, ahbmi, ahbmo, ahbsi, ahbso);

----------------------------------------------------------------------
---  LEON3 processor and DSU -----------------------------------------
----------------------------------------------------------------------

  -- LEON3 processor
  leon3gen : if CFG_LEON3 = 1 generate
    cpu : for i in 0 to CFG_NCPU-1 generate
      u0 : leon3s
        generic map (i, fabtech, memtech, CFG_NWIN, CFG_DSU, CFG_FPU, CFG_V8,
                     0, CFG_MAC, pclow, CFG_NOTAG, CFG_NWP, CFG_ICEN, CFG_IREPL, CFG_ISETS, CFG_ILINE,
                     CFG_ISETSZ, CFG_ILOCK, CFG_DCEN, CFG_DREPL, CFG_DSETS, CFG_DLINE, CFG_DSETSZ,
                     CFG_DLOCK, CFG_DSNOOP, CFG_ILRAMEN, CFG_ILRAMSZ, CFG_ILRAMADDR, CFG_DLRAMEN,
                     CFG_DLRAMSZ, CFG_DLRAMADDR, CFG_MMUEN, CFG_ITLBNUM, CFG_DTLBNUM, CFG_TLB_TYPE, CFG_TLB_REP,
                     CFG_LDDEL, disas, CFG_ITBSZ, CFG_PWD, CFG_SVT, CFG_RSTADDR,
                     CFG_NCPU-1, CFG_DFIXED, CFG_SCAN, CFG_MMU_PAGE, CFG_BP, CFG_NP_ASI, CFG_WRPSR,
                     CFG_REX, CFG_ALTWIN)
        port map (clkm, rstn, ahbmi, ahbmo(i), ahbsi, ahbso, irqi(i), irqo(i), dbgi(i), dbgo(i));
    end generate;

    led(3)  <= not dbgo(0).error;
    led(2)  <= not dsuo.active;

    -- LEON3 Debug Support Unit
    dsugen : if CFG_DSU = 1 generate
      dsu0 : dsu3
        generic map (hindex => 2, haddr => 16#900#, hmask => 16#F00#, ahbpf => CFG_AHBPF,
                     ncpu   => CFG_NCPU, tbits => 30, tech => memtech, irq => 0, kbytes => CFG_ATBSZ)
        port map (rstn, clkm, ahbmi, ahbsi, ahbso(2), dbgo, dbgi, dsui, dsuo);

      --dsubre_pad : inpad generic map (tech  => padtech) port map (dsubre, dsui.break);

      dsui.enable <= '1';

    end generate;
  end generate;
  nodsu : if CFG_DSU = 0 generate
    ahbso(2) <= ahbs_none; dsuo.tstop <= '0'; dsuo.active <= '0';
  end generate;

  -- Debug UART
  dcomgen : if CFG_AHB_UART = 1 generate
    dcom0 : ahbuart
      generic map (hindex => CFG_NCPU, pindex => 4, paddr => 7)
      port map (rstn, clkm, dui, duo, apbi, apbo(4), ahbmi, ahbmo(CFG_NCPU));
    --dsurx_pad : inpad generic map (tech  => padtech) port map (uart_tx_in, dui.rxd);
    --dsutx_pad : outpad generic map (tech => padtech) port map (uart_rx_out, duo.txd);
    dbg_uart_txd <= duo.txd;
    dui.rxd <= dbg_uart_rxd;
    led(0) <= not dui.rxd;
    led(1) <= not duo.txd;
  end generate;
  nouah : if CFG_AHB_UART = 0 generate apbo(4) <= apb_none; end generate;

  ahbjtaggen0 :if CFG_AHB_JTAG = 1 generate
    ahbjtag0 : ahbjtag generic map(tech => fabtech, hindex => CFG_NCPU+CFG_AHB_UART)
      port map(rstn, clkm, dbg_jtag_tck, dbg_jtag_tms, dbg_jtag_tdi, dbg_jtag_tdo, ahbmi, ahbmo(CFG_NCPU+CFG_AHB_UART),
               open, open, open, open, open, open, open, gnd);
  end generate;

----------------------------------------------------------------------
---  DDR3 Memory controller ------------------------------------------
----------------------------------------------------------------------
  
--  nomig : if (CFG_MIG_7SERIES = 0) generate end generate;

  mig_gen : if (CFG_MIG_7SERIES = 1) generate
    gen_mig : if (USE_MIG_INTERFACE_MODEL /= true) generate
      ddrc : ahb2mig_7series_ddr3_dq16_ad14_ba3 generic map(
          hindex => 5, haddr => 16#400#, hmask => 16#F00#, pindex => 5, paddr => 5,
          SIM_BYPASS_INIT_CAL => SIM_BYPASS_INIT_CAL, SIMULATION => SIMULATION,
          USE_MIG_INTERFACE_MODEL => USE_MIG_INTERFACE_MODEL)
        port map(
          ddr3_dq         => ddr3_dq,
          ddr3_dqs_p      => ddr3_dqs_p,
          ddr3_dqs_n      => ddr3_dqs_n,
          ddr3_addr       => ddr3_addr,
          ddr3_ba         => ddr3_ba,
          ddr3_ras_n      => ddr3_ras_n,
          ddr3_cas_n      => ddr3_cas_n,
          ddr3_we_n       => ddr3_we_n,
          ddr3_reset_n    => ddr3_reset_n,
          ddr3_ck_p       => ddr3_ck_p,
          ddr3_ck_n       => ddr3_ck_n,
          ddr3_cke        => ddr3_cke,
          ddr3_dm         => ddr3_dm,
          ddr3_odt        => ddr3_odt,
          ahbsi           => ahbsi,
          ahbso           => ahbso(5),
          apbi            => apbi,
          apbo            => apbo(5),
          calib_done      => calib_done,
          rst_n_syn       => migrstn,
          rst_n_async     => cgo1.clklock,--rstraw,
          clk_amba        => clkm,
          sys_clk_i       => clkinmig,
          clk_ref_i       => clkinmig,
          ui_clk          => clkm, -- 100 MHz clk , DDR at 400 MHz
          ui_clk_sync_rst => open);
  
--    clkgenmigref0 : clkgen
--      generic map (clktech, 25, 6, 0, CFG_CLK_NOFB, 0, 0, 0, 48000)
--      port map (sysclk, sysclk, clkref, open, open, open, open, cgi, cgo, open, open, open);
  
    clkgenmigin : clkgen
      generic map (clktech, 25, 6, 0, CFG_CLK_NOFB, 0, 0, 0, 48000)
      port map (sysclk, sysclk, clkinmig, open, open, open, open, cgi, cgo1, open, open, open);
  end generate gen_mig;
  
  gen_mig_model : if (USE_MIG_INTERFACE_MODEL = true) generate
    -- pragma translate_off
  
    mig_ahbram : ahbram_sim
      generic map (
        hindex   => 5,
        haddr    => 16#400#,
        hmask    => 16#F80#,
        tech     => 0,
        kbytes   => 1000,
        pipe     => 0,
        maccsz   => AHBDW,
        fname    => "ram.srec"
        )
      port map(
        rst     => rstn,
        clk     => clkm,
        ahbsi   => ahbsi,
        ahbso   => ahbso(5)
        );
  
    ddr3_dq           <= (others => 'Z');
    ddr3_dqs_p        <= (others => 'Z');
    ddr3_dqs_n        <= (others => 'Z');
    ddr3_addr         <= (others => '0');
    ddr3_ba           <= (others => '0');
    ddr3_ras_n        <= '0';
    ddr3_cas_n        <= '0';
    ddr3_we_n         <= '0';
    ddr3_ck_p         <= (others => '0');
    ddr3_ck_n         <= (others => '0');
    ddr3_cke          <= (others => '0');
    ddr3_dm           <= (others => '0');
    ddr3_odt          <= (others => '0');
  
    --calib_done        : out   std_logic;
    calib_done <= '1';
       
    --ui_clk            : out   std_logic;
    clkm <= not clkm after 10.0 ns;
      
    --ui_clk_sync_rst   : out   std_logic
    -- n/a
    -- pragma translate_on
  
  end generate gen_mig_model;    end generate;

----------------------------------------------------------------------
---  SPI Memory controller -------------------------------------------
----------------------------------------------------------------------
  spi_gen: if CFG_SPIMCTRL = 1 generate
    -- OPTIONALY set the offset generic (only affect reads).
    -- The first 4MB are used for loading the FPGA.
    -- For dual ouptut: readcmd => 16#3B#, dualoutput => 1
    spimctrl1 : spimctrl
      generic map (hindex => 7, hirq => 7, faddr => 16#000#, fmask => 16#ff0#,
                   ioaddr => 16#700#, iomask => 16#fff#, spliten => CFG_SPLIT,
                   sdcard => CFG_SPIMCTRL_SDCARD, readcmd => CFG_SPIMCTRL_READCMD,
                   dummybyte => CFG_SPIMCTRL_DUMMYBYTE, 
                   dualoutput => CFG_SPIMCTRL_DUALOUTPUT, scaler => CFG_SPIMCTRL_SCALER,
                   altscaler => CFG_SPIMCTRL_ASCALER)
      port map (rstn, clkm, ahbsi, ahbso(7), spmi, spmo);

      spi_mosi <= spmo.mosi;
      spmi.miso <= spi_miso;
      spi_cs <= spmo.csn;
      spi_sck <= spmo.sck;
    -- qspi_dq(3) <= '1'; qspi_dq(2) <= '1';
    --spi_mosi_pad : outpad generic map (tech => padtech)
    --  port map (spi_mosi, spmo.mosi);
    --spi_miso_pad : inpad generic map (tech => padtech)
    --  port map (spi_miso, spmi.miso);
    --spi_slvsel0_pad : outpad generic map (tech => padtech)
    --  port map (spi_cs, spmo.csn);
    --spi_sck_pad : outpad generic map (tech => padtech)
    --  port map (spi_sck, spmo.sck);
  end generate;

  nospi: if CFG_SPIMCTRL = 0 generate
      ahbso(7) <= ahbs_none;
  end generate;

----------------------------------------------------------------------
---  APB Bridge and various periherals -------------------------------
----------------------------------------------------------------------

  -- APB Bridge
  apb0 : apbctrl
    generic map (hindex => 1, haddr => CFG_APBADDR)
    port map (rstn, clkm, ahbsi, ahbso(1), apbi, apbo);

  -- Interrupt controller
  irqctrl : if CFG_IRQ3_ENABLE /= 0 generate
    irqctrl0 : irqmp
      generic map (pindex => 2, paddr => 2, ncpu => CFG_NCPU)
      port map (rstn, clkm, apbi, apbo(2), irqo, irqi);
  end generate;
  irq3 : if CFG_IRQ3_ENABLE = 0 generate
    x : for i in 0 to CFG_NCPU-1 generate
      irqi(i).irl <= "0000";
    end generate;
    apbo(2) <= apb_none;
  end generate;

  -- Timer Unit
  gpt : if CFG_GPT_ENABLE /= 0 generate
    timer0 : gptimer
      generic map (pindex => 3, paddr => 3, pirq => CFG_GPT_IRQ,
                   sepirq => CFG_GPT_SEPIRQ, sbits => CFG_GPT_SW,
                   ntimers => CFG_GPT_NTIM, nbits  => CFG_GPT_TW)
      port map (rstn, clkm, apbi, apbo(3), gpti, open);
    gpti <= gpti_dhalt_drive(dsuo.tstop);
  end generate;
  notim : if CFG_GPT_ENABLE = 0 generate apbo(3) <= apb_none; end generate;


  -- UARTs
  ua1 : if CFG_UART1_ENABLE /= 0 generate
    uart1 : apbuart                     -- UART 1
      generic map (pindex   => 1, paddr => 1, pirq => 2, console => dbguart, fifosize => CFG_UART1_FIFO)
      port map (rstn, clkm, apbi, apbo(1), u1i, u1o);
    u1i.rxd    <= radio_rxd;
    u1i.ctsn   <= '0';
    u1i.extclk <= '0';
    radio_txd       <= u1o.txd;
  end generate;
  noua1 : if CFG_UART1_ENABLE = 0 generate apbo(1) <= apb_none; end generate;
  
  ua2 : if CFG_UART2_ENABLE /= 0 generate
    uart2 : apbuart                     -- UART 2
      generic map (pindex   => 6, paddr => 6, pirq => 6, fifosize => CFG_UART2_FIFO)
      port map (rstn, clkm, apbi, apbo(6), u2i, u2o);
    u2i.rxd    <= console_rxd;
    u2i.ctsn   <= '0';
    u2i.extclk <= '0';
    console_txd       <= u2o.txd;
  end generate;
  noua2 : if CFG_UART2_ENABLE = 0 generate apbo(6) <= apb_none; end generate;
  
  ua3 : if CFG_UART3_ENABLE /= 0 generate
    uart3 : apbuart                     -- UART 3
      generic map (pindex   => 9, paddr => 9, pirq => 9, fifosize => CFG_UART3_FIFO)
      port map (rstn, clkm, apbi, apbo(9), u3i, u3o);
    u3i.rxd    <= eyassat_rxd;
    u3i.ctsn   <= '0';
    u3i.extclk <= '0';
    eyassat_txd  <= u3o.txd;
  end generate;
  noua3 : if CFG_UART3_ENABLE = 0 generate apbo(9) <= apb_none; end generate;
  
  ua4 : if CFG_UART4_ENABLE /= 0 generate
    uart4 : apbuart                     -- UART 4
      generic map (pindex   => 10, paddr => 10, pirq => 10, fifosize => CFG_UART4_FIFO)
      port map (rstn, clkm, apbi, apbo(10), u4i, u4o);
    u4i.rxd    <= payload_rxd;
    u4i.ctsn   <= '0';
    u4i.extclk <= '0';
    payload_txd  <= u4o.txd;
  end generate;
  noua4 : if CFG_UART4_ENABLE = 0 generate apbo(10) <= apb_none; end generate;
  
-- SPICTRL0 -- wired as slave
  spictrl0 : spictrl 
    generic map (pindex => 11, paddr => 11, pirq => 11, fdepth=> 1, slvselen => 0, slvselsz => 1)
    port map (rstn, clkm, apbi, apbo(11), spi0_spii, spi0_spio, open);
  spi0_miso <= spi0_spio.miso;
  spi0_spii.mosi <= spi0_mosi;
  spi0_spii.sck <= spi0_sck;
  spi0_spii.spisel <= spi0_sel;

-- I2C Master
  i2cm: if CFG_I2C_ENABLE = 1 generate  -- I2C master
    i2c0 : i2cmst
    generic map (pindex => 12, paddr => 12, pmask => 16#FFF#,
                 pirq => 12, filter => I2C_FILTER)
    port map (rstn, clkm, apbi, apbo(12), i2c0_i2ci, i2c0_i2co);
        --i2c_scl_pad : iopad generic map (tech => padtech)
    --  port map (i2c_scl, i2co.scl, i2co.scloen, i2ci.scl);
    --i2c_sda_pad : iopad generic map (tech => padtech)
    --  port map (i2c_sda, i2co.sda, i2co.sdaoen, i2ci.sda);
    --i2c0_scl_o <= i2c0_i2co.scl when (i2c0_i2co.scloen = '0') else 'Z';
    --i2c0_sda_o <= i2c0_i2co.sda when (i2c0_i2co.sdaoen = '0') else 'Z';
    
    i2c0_scl_oen <= i2c0_i2co.scloen;
    i2c0_sda_oen <= i2c0_i2co.sdaoen;
    
    i2c0_scl_o <= i2c0_i2co.scl;
    i2c0_sda_o <= i2c0_i2co.sda;
    
    i2c0_i2ci.scl <= i2c0_scl_i; 
    i2c0_i2ci.sda <= i2c0_sda_i; 
  end generate i2cm;

  -- GPIO
  grgpio0: grgpio
    generic map(pindex => 8, paddr => 8, imask => 16#00F0#, nbits => 14)
    port map(rst => rstn, clk => clkm, apbi => apbi, apbo => apbo(8),
    gpioi => gpioi, gpioo => gpioo);
    --gpio_pads : iopadvv generic map (tech => padtech, width => 14)
    --  port map (gpio, gpioo.dout(13 downto 0), gpioo.oen(13 downto 0),
    --            gpioi.din(13 downto 0));
    gpio_dout(13 downto 0) <= gpioo.dout(13 downto 0);
    gpioi.din(13 downto 0) <= gpio_din(13 downto 0);
    gpio_oen(13 downto 0) <= gpioo.oen(13 downto 0);

-----------------------------------------------------------------------
---  AHB ROM ----------------------------------------------------------
-----------------------------------------------------------------------

  bpromgen : if CFG_AHBROMEN /= 0 generate
    brom : entity work.ahbrom
      generic map (hindex => 6, haddr => CFG_AHBRODDR, pipe => CFG_AHBROPIP)
      port map ( rstn, clkm, ahbsi, ahbso(6));
  end generate;
  nobpromgen : if CFG_AHBROMEN = 0 generate
     ahbso(6) <= ahbs_none;
  end generate;

-----------------------------------------------------------------------
---  AHB RAM ----------------------------------------------------------
-----------------------------------------------------------------------

  ahbramgen : if CFG_AHBRAMEN = 1 generate
    ahbram0 : ahbram
      generic map (hindex => 3, haddr => CFG_AHBRADDR, tech => CFG_MEMTECH,
                   kbytes => CFG_AHBRSZ, pipe => CFG_AHBRPIPE)
      port map (rstn, clkm, ahbsi, ahbso(3));
  end generate;
  nram : if CFG_AHBRAMEN = 0 generate ahbso(3) <= ahbs_none; end generate;

-----------------------------------------------------------------------
--  Test report module, only used for simulation ----------------------
-----------------------------------------------------------------------

--pragma translate_off
  test0 : ahbrep generic map (hindex => 4, haddr => 16#200#)
    port map (rstn, clkm, ahbsi, ahbso(4));
--pragma translate_on

-----------------------------------------------------------------------
---  Drive unused bus elements  ---------------------------------------
-----------------------------------------------------------------------

  nam1 : for i in (CFG_NCPU+CFG_AHB_UART+CFG_AHB_JTAG+CFG_GRETH+1) to NAHBMST-1 generate
    ahbmo(i) <= ahbm_none;
  end generate;

-----------------------------------------------------------------------
---  Boot message  ----------------------------------------------------
-----------------------------------------------------------------------

-- pragma translate_off
  x : report_design
    generic map (
      msg1 => "LEON3 Demonstration design for Digilent Nexys Video board",
      fabtech => tech_table(fabtech), memtech => tech_table(memtech),
      mdel => 1
      );
-- pragma translate_on

end rtl;

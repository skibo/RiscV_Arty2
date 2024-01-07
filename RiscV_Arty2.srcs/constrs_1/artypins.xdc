set_property -dict { PACKAGE_PIN G18   IOSTANDARD LVCMOS33 } [get_ports { eth_ref_clk }]; #IO_L22P_T3_A17_15 Sch=eth_ref_clk

##Pmod Header JA

set_property -dict { PACKAGE_PIN A11   IOSTANDARD LVCMOS33 } [get_ports { uart1_tx }]; #IO_L4N_T0_15 Sch=ja[3]
set_property -dict { PACKAGE_PIN D12   IOSTANDARD LVCMOS33 } [get_ports { uart1_rx }]; #IO_L6P_T0_15 Sch=ja[4]


# CFGBVS
set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]

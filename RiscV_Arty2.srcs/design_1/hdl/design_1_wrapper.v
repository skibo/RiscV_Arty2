//Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2017.3 (lin64) Build 2018833 Wed Oct  4 19:58:07 MDT 2017
//Date        : Sat Jan  6 20:32:21 2018
//Host        : robleda running 64-bit Ubuntu 16.04.3 LTS
//Command     : generate_target design_1_wrapper.bd
//Design      : design_1_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module design_1_wrapper
   (dip_switches_4bits_tri_i,
    led_4bits_tri_io,
    push_buttons_4bits_tri_i,
    reset,
    rgb_led_tri_io,
    sys_clock,
    usb_uart_rxd,
    usb_uart_txd);
  input [3:0]dip_switches_4bits_tri_i;
  inout [3:0]led_4bits_tri_io;
  input [3:0]push_buttons_4bits_tri_i;
  input reset;
  inout [11:0]rgb_led_tri_io;
  input sys_clock;
  input usb_uart_rxd;
  output usb_uart_txd;

  wire [3:0]dip_switches_4bits_tri_i;
  wire [0:0]led_4bits_tri_i_0;
  wire [1:1]led_4bits_tri_i_1;
  wire [2:2]led_4bits_tri_i_2;
  wire [3:3]led_4bits_tri_i_3;
  wire [0:0]led_4bits_tri_io_0;
  wire [1:1]led_4bits_tri_io_1;
  wire [2:2]led_4bits_tri_io_2;
  wire [3:3]led_4bits_tri_io_3;
  wire [0:0]led_4bits_tri_o_0;
  wire [1:1]led_4bits_tri_o_1;
  wire [2:2]led_4bits_tri_o_2;
  wire [3:3]led_4bits_tri_o_3;
  wire [0:0]led_4bits_tri_t_0;
  wire [1:1]led_4bits_tri_t_1;
  wire [2:2]led_4bits_tri_t_2;
  wire [3:3]led_4bits_tri_t_3;
  wire [3:0]push_buttons_4bits_tri_i;
  wire reset;
  wire [0:0]rgb_led_tri_i_0;
  wire [1:1]rgb_led_tri_i_1;
  wire [10:10]rgb_led_tri_i_10;
  wire [11:11]rgb_led_tri_i_11;
  wire [2:2]rgb_led_tri_i_2;
  wire [3:3]rgb_led_tri_i_3;
  wire [4:4]rgb_led_tri_i_4;
  wire [5:5]rgb_led_tri_i_5;
  wire [6:6]rgb_led_tri_i_6;
  wire [7:7]rgb_led_tri_i_7;
  wire [8:8]rgb_led_tri_i_8;
  wire [9:9]rgb_led_tri_i_9;
  wire [0:0]rgb_led_tri_io_0;
  wire [1:1]rgb_led_tri_io_1;
  wire [10:10]rgb_led_tri_io_10;
  wire [11:11]rgb_led_tri_io_11;
  wire [2:2]rgb_led_tri_io_2;
  wire [3:3]rgb_led_tri_io_3;
  wire [4:4]rgb_led_tri_io_4;
  wire [5:5]rgb_led_tri_io_5;
  wire [6:6]rgb_led_tri_io_6;
  wire [7:7]rgb_led_tri_io_7;
  wire [8:8]rgb_led_tri_io_8;
  wire [9:9]rgb_led_tri_io_9;
  wire [0:0]rgb_led_tri_o_0;
  wire [1:1]rgb_led_tri_o_1;
  wire [10:10]rgb_led_tri_o_10;
  wire [11:11]rgb_led_tri_o_11;
  wire [2:2]rgb_led_tri_o_2;
  wire [3:3]rgb_led_tri_o_3;
  wire [4:4]rgb_led_tri_o_4;
  wire [5:5]rgb_led_tri_o_5;
  wire [6:6]rgb_led_tri_o_6;
  wire [7:7]rgb_led_tri_o_7;
  wire [8:8]rgb_led_tri_o_8;
  wire [9:9]rgb_led_tri_o_9;
  wire [0:0]rgb_led_tri_t_0;
  wire [1:1]rgb_led_tri_t_1;
  wire [10:10]rgb_led_tri_t_10;
  wire [11:11]rgb_led_tri_t_11;
  wire [2:2]rgb_led_tri_t_2;
  wire [3:3]rgb_led_tri_t_3;
  wire [4:4]rgb_led_tri_t_4;
  wire [5:5]rgb_led_tri_t_5;
  wire [6:6]rgb_led_tri_t_6;
  wire [7:7]rgb_led_tri_t_7;
  wire [8:8]rgb_led_tri_t_8;
  wire [9:9]rgb_led_tri_t_9;
  wire sys_clock;
  wire usb_uart_rxd;
  wire usb_uart_txd;

  design_1 design_1_i
       (.dip_switches_4bits_tri_i(dip_switches_4bits_tri_i),
        .led_4bits_tri_i({led_4bits_tri_i_3,led_4bits_tri_i_2,led_4bits_tri_i_1,led_4bits_tri_i_0}),
        .led_4bits_tri_o({led_4bits_tri_o_3,led_4bits_tri_o_2,led_4bits_tri_o_1,led_4bits_tri_o_0}),
        .led_4bits_tri_t({led_4bits_tri_t_3,led_4bits_tri_t_2,led_4bits_tri_t_1,led_4bits_tri_t_0}),
        .push_buttons_4bits_tri_i(push_buttons_4bits_tri_i),
        .reset(reset),
        .rgb_led_tri_i({rgb_led_tri_i_11,rgb_led_tri_i_10,rgb_led_tri_i_9,rgb_led_tri_i_8,rgb_led_tri_i_7,rgb_led_tri_i_6,rgb_led_tri_i_5,rgb_led_tri_i_4,rgb_led_tri_i_3,rgb_led_tri_i_2,rgb_led_tri_i_1,rgb_led_tri_i_0}),
        .rgb_led_tri_o({rgb_led_tri_o_11,rgb_led_tri_o_10,rgb_led_tri_o_9,rgb_led_tri_o_8,rgb_led_tri_o_7,rgb_led_tri_o_6,rgb_led_tri_o_5,rgb_led_tri_o_4,rgb_led_tri_o_3,rgb_led_tri_o_2,rgb_led_tri_o_1,rgb_led_tri_o_0}),
        .rgb_led_tri_t({rgb_led_tri_t_11,rgb_led_tri_t_10,rgb_led_tri_t_9,rgb_led_tri_t_8,rgb_led_tri_t_7,rgb_led_tri_t_6,rgb_led_tri_t_5,rgb_led_tri_t_4,rgb_led_tri_t_3,rgb_led_tri_t_2,rgb_led_tri_t_1,rgb_led_tri_t_0}),
        .sys_clock(sys_clock),
        .usb_uart_rxd(usb_uart_rxd),
        .usb_uart_txd(usb_uart_txd));
  IOBUF led_4bits_tri_iobuf_0
       (.I(led_4bits_tri_o_0),
        .IO(led_4bits_tri_io[0]),
        .O(led_4bits_tri_i_0),
        .T(led_4bits_tri_t_0));
  IOBUF led_4bits_tri_iobuf_1
       (.I(led_4bits_tri_o_1),
        .IO(led_4bits_tri_io[1]),
        .O(led_4bits_tri_i_1),
        .T(led_4bits_tri_t_1));
  IOBUF led_4bits_tri_iobuf_2
       (.I(led_4bits_tri_o_2),
        .IO(led_4bits_tri_io[2]),
        .O(led_4bits_tri_i_2),
        .T(led_4bits_tri_t_2));
  IOBUF led_4bits_tri_iobuf_3
       (.I(led_4bits_tri_o_3),
        .IO(led_4bits_tri_io[3]),
        .O(led_4bits_tri_i_3),
        .T(led_4bits_tri_t_3));
  IOBUF rgb_led_tri_iobuf_0
       (.I(rgb_led_tri_o_0),
        .IO(rgb_led_tri_io[0]),
        .O(rgb_led_tri_i_0),
        .T(rgb_led_tri_t_0));
  IOBUF rgb_led_tri_iobuf_1
       (.I(rgb_led_tri_o_1),
        .IO(rgb_led_tri_io[1]),
        .O(rgb_led_tri_i_1),
        .T(rgb_led_tri_t_1));
  IOBUF rgb_led_tri_iobuf_10
       (.I(rgb_led_tri_o_10),
        .IO(rgb_led_tri_io[10]),
        .O(rgb_led_tri_i_10),
        .T(rgb_led_tri_t_10));
  IOBUF rgb_led_tri_iobuf_11
       (.I(rgb_led_tri_o_11),
        .IO(rgb_led_tri_io[11]),
        .O(rgb_led_tri_i_11),
        .T(rgb_led_tri_t_11));
  IOBUF rgb_led_tri_iobuf_2
       (.I(rgb_led_tri_o_2),
        .IO(rgb_led_tri_io[2]),
        .O(rgb_led_tri_i_2),
        .T(rgb_led_tri_t_2));
  IOBUF rgb_led_tri_iobuf_3
       (.I(rgb_led_tri_o_3),
        .IO(rgb_led_tri_io[3]),
        .O(rgb_led_tri_i_3),
        .T(rgb_led_tri_t_3));
  IOBUF rgb_led_tri_iobuf_4
       (.I(rgb_led_tri_o_4),
        .IO(rgb_led_tri_io[4]),
        .O(rgb_led_tri_i_4),
        .T(rgb_led_tri_t_4));
  IOBUF rgb_led_tri_iobuf_5
       (.I(rgb_led_tri_o_5),
        .IO(rgb_led_tri_io[5]),
        .O(rgb_led_tri_i_5),
        .T(rgb_led_tri_t_5));
  IOBUF rgb_led_tri_iobuf_6
       (.I(rgb_led_tri_o_6),
        .IO(rgb_led_tri_io[6]),
        .O(rgb_led_tri_i_6),
        .T(rgb_led_tri_t_6));
  IOBUF rgb_led_tri_iobuf_7
       (.I(rgb_led_tri_o_7),
        .IO(rgb_led_tri_io[7]),
        .O(rgb_led_tri_i_7),
        .T(rgb_led_tri_t_7));
  IOBUF rgb_led_tri_iobuf_8
       (.I(rgb_led_tri_o_8),
        .IO(rgb_led_tri_io[8]),
        .O(rgb_led_tri_i_8),
        .T(rgb_led_tri_t_8));
  IOBUF rgb_led_tri_iobuf_9
       (.I(rgb_led_tri_o_9),
        .IO(rgb_led_tri_io[9]),
        .O(rgb_led_tri_i_9),
        .T(rgb_led_tri_t_9));
endmodule

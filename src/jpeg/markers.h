// IJG JPEG 6a routines and tables the original links, paired by their IJG names.

#ifndef HYPERBLADE_JPEG_MARKERS_H
#define HYPERBLADE_JPEG_MARKERS_H

// jdapimin.c
// LIBRARY: HYPERX 0x00494070
// jpeg_CreateDecompress

// LIBRARY: HYPERX 0x00494121
// jpeg_destroy_decompress

// LIBRARY: HYPERX 0x0049412c
// jpeg_set_marker_processor

// LIBRARY: HYPERX 0x00494189
// jpeg_read_header

// LIBRARY: HYPERX 0x004941eb
// jpeg_consume_input

// LIBRARY: HYPERX 0x00494287
// default_decompress_parms

// LIBRARY: HYPERX 0x0049440b
// jpeg_finish_decompress

// jdapistd.c
// LIBRARY: HYPERX 0x004944a2
// jpeg_start_decompress

// LIBRARY: HYPERX 0x00494565
// output_pass_setup

// LIBRARY: HYPERX 0x0049460f
// jpeg_read_scanlines

// jdatasrc.c
// LIBRARY: HYPERX 0x00494d34
// jpeg_stdio_src

// LIBRARY: HYPERX 0x00494d98
// init_source

// LIBRARY: HYPERX 0x00494da4
// fill_input_buffer

// LIBRARY: HYPERX 0x00494e0f
// skip_input_data

// cdjpeg.c
// LIBRARY: HYPERX 0x00494e3c
// signal_catcher

// LIBRARY: HYPERX 0x00494e62
// enable_signal_catcher

// jerror.c
// LIBRARY: HYPERX 0x00494e86
// jpeg_std_error

// LIBRARY: HYPERX 0x00494ed2
// error_exit

// LIBRARY: HYPERX 0x00494eed
// output_message

// LIBRARY: HYPERX 0x00494f21
// emit_message

// LIBRARY: HYPERX 0x00494f52
// format_message

// LIBRARY: HYPERX 0x00494fea
// reset_error_mgr

// jdinput.c
// LIBRARY: HYPERX 0x00494ffb
// jinit_input_controller

// LIBRARY: HYPERX 0x0049503c
// start_input_pass

// LIBRARY: HYPERX 0x00495079
// per_scan_setup

// LIBRARY: HYPERX 0x004951f9
// latch_quant_tables

// LIBRARY: HYPERX 0x00495291
// finish_input_pass

// LIBRARY: HYPERX 0x004952a2
// consume_markers

// LIBRARY: HYPERX 0x00495337
// initial_setup

// LIBRARY: HYPERX 0x004954fd
// reset_input_controller

// jdmarker.c
// LIBRARY: HYPERX 0x00495534
// jpeg_resync_to_restart

// LIBRARY: HYPERX 0x00495622
// next_marker

// LIBRARY: HYPERX 0x004956f9
// jinit_marker_reader

// LIBRARY: HYPERX 0x00495776
// get_app0

// LIBRARY: HYPERX 0x004959b2
// get_app14

// LIBRARY: HYPERX 0x00495b2e
// skip_variable

// LIBRARY: HYPERX 0x00495bca
// read_markers

// LIBRARY: HYPERX 0x00495db3
// get_soi

// LIBRARY: HYPERX 0x00495e50
// get_sof

// LIBRARY: HYPERX 0x00496150
// get_sos

// LIBRARY: HYPERX 0x0049643e
// get_dac

// LIBRARY: HYPERX 0x004965a4
// get_dht

// LIBRARY: HYPERX 0x00496830
// get_dqt

// LIBRARY: HYPERX 0x00496a8b
// get_dri

// LIBRARY: HYPERX 0x00496b6a
// first_marker

// LIBRARY: HYPERX 0x00496c06
// read_restart_marker

// LIBRARY: HYPERX 0x00496c84
// reset_marker_reader

// jmemmgr.c
// LIBRARY: HYPERX 0x00496cb2
// jinit_memory_mgr

// LIBRARY: HYPERX 0x00496dbe
// alloc_small

// LIBRARY: HYPERX 0x00496edc
// alloc_large

// LIBRARY: HYPERX 0x00496f8d
// alloc_sarray

// LIBRARY: HYPERX 0x00497025
// alloc_barray

// LIBRARY: HYPERX 0x004970c9
// request_virt_sarray

// LIBRARY: HYPERX 0x0049712a
// request_virt_barray

// LIBRARY: HYPERX 0x0049718b
// realize_virt_arrays

// LIBRARY: HYPERX 0x0049730e
// access_virt_sarray

// LIBRARY: HYPERX 0x0049742e
// do_sarray_io

// LIBRARY: HYPERX 0x004974be
// access_virt_barray

// LIBRARY: HYPERX 0x004975e3
// do_barray_io

// LIBRARY: HYPERX 0x00497674
// free_pool

// LIBRARY: HYPERX 0x0049775b
// self_destruct

// jcomapi.c
// LIBRARY: HYPERX 0x0049778b
// jpeg_abort

// LIBRARY: HYPERX 0x004977ad
// jpeg_destroy

// LIBRARY: HYPERX 0x004977c8
// jpeg_alloc_quant_table

// LIBRARY: HYPERX 0x004977e4
// jpeg_alloc_huff_table

// jdmaster.c
// LIBRARY: HYPERX 0x00497800
// jpeg_calc_output_dimensions

// LIBRARY: HYPERX 0x004979f8
// use_merged_upsample

// LIBRARY: HYPERX 0x00497a6d
// jinit_master_decompress

// LIBRARY: HYPERX 0x00497a9e
// master_selection

// LIBRARY: HYPERX 0x00497c54
// prepare_range_limit_table

// LIBRARY: HYPERX 0x00497cda
// prepare_for_output_pass

// LIBRARY: HYPERX 0x00497e05
// finish_output_pass

// jutils.c
// LIBRARY: HYPERX 0x00497e26
// jdiv_round_up

// LIBRARY: HYPERX 0x00497e36
// jround_up

// LIBRARY: HYPERX 0x00497e4e
// jcopy_sample_rows

// LIBRARY: HYPERX 0x00497e8c
// jcopy_block_row

// LIBRARY: HYPERX 0x00497ea5
// jzero_far

// jmemansi.c
// LIBRARY: HYPERX 0x00497eb8
// jpeg_free_small

// LIBRARY: HYPERX 0x00497ec3
// jpeg_get_small

// LIBRARY: HYPERX 0x00497ece
// jpeg_mem_available

// LIBRARY: HYPERX 0x00497edd
// jpeg_open_backing_store

// LIBRARY: HYPERX 0x00497f2d
// read_backing_store

// LIBRARY: HYPERX 0x00497f86
// write_backing_store

// LIBRARY: HYPERX 0x00497fdf
// close_backing_store

// LIBRARY: HYPERX 0x00497fed
// jpeg_mem_init

// LIBRARY: HYPERX 0x00497ff3
// jpeg_mem_term

// jdmainct.c
// LIBRARY: HYPERX 0x00497ff4
// jinit_d_main_controller

// LIBRARY: HYPERX 0x004980c9
// alloc_funny_pointers

// LIBRARY: HYPERX 0x00498176
// start_pass_main

// LIBRARY: HYPERX 0x004981de
// make_funny_pointers

// LIBRARY: HYPERX 0x0049831e
// process_data_simple_main

// LIBRARY: HYPERX 0x00498386
// process_data_context_main

// LIBRARY: HYPERX 0x00498498
// set_wraparound_pointers

// LIBRARY: HYPERX 0x00498582
// set_bottom_pointers

// LIBRARY: HYPERX 0x0049861c
// process_data_crank_post

// jdcoefct.c
// LIBRARY: HYPERX 0x0049863f
// jinit_d_coef_controller

// LIBRARY: HYPERX 0x00498737
// start_input_pass

// LIBRARY: HYPERX 0x0049874a
// start_iMCU_row

// LIBRARY: HYPERX 0x0049878d
// start_output_pass

// LIBRARY: HYPERX 0x004987ca
// decompress_onepass

// LIBRARY: HYPERX 0x004989ae
// dummy_consume_data

// LIBRARY: HYPERX 0x004989b1
// consume_data

// LIBRARY: HYPERX 0x00498b6a
// decompress_data

// LIBRARY: HYPERX 0x00498cd6
// smoothing_ok

// LIBRARY: HYPERX 0x00498dbe
// decompress_smooth_data

// jdhuff.c
// LIBRARY: HYPERX 0x00499319
// jpeg_make_d_derived_tbl

// LIBRARY: HYPERX 0x004994e6
// jpeg_fill_bit_buffer

// LIBRARY: HYPERX 0x004995cc
// jpeg_huff_decode

// LIBRARY: HYPERX 0x004996b4
// jinit_huff_decoder

// LIBRARY: HYPERX 0x004996ee
// start_pass_huff_decoder

// LIBRARY: HYPERX 0x0049981a
// decode_mcu

// LIBRARY: HYPERX 0x00499bd4
// process_restart

// jdphuff.c
// LIBRARY: HYPERX 0x00499c36
// jinit_phuff_decoder

// LIBRARY: HYPERX 0x00499c97
// start_pass_phuff_decoder

// LIBRARY: HYPERX 0x00499f40
// decode_mcu_DC_first

// LIBRARY: HYPERX 0x0049a144
// process_restart

// LIBRARY: HYPERX 0x0049a1a9
// decode_mcu_AC_first

// LIBRARY: HYPERX 0x0049a3d5
// decode_mcu_DC_refine

// LIBRARY: HYPERX 0x0049a4bf
// decode_mcu_AC_refine

// jddctmgr.c
// LIBRARY: HYPERX 0x0049a85e
// jinit_inverse_dct

// LIBRARY: HYPERX 0x0049a8d3
// start_pass

// jdpostct.c
// LIBRARY: HYPERX 0x0049aa96
// jinit_d_post_controller

// LIBRARY: HYPERX 0x0049ab19
// start_pass_dpost

// LIBRARY: HYPERX 0x0049abbf
// post_process_1pass

// LIBRARY: HYPERX 0x0049ac24
// post_process_prepass

// LIBRARY: HYPERX 0x0049acb5
// post_process_2pass

// jdsample.c
// LIBRARY: HYPERX 0x0049ad41
// jinit_upsampler

// LIBRARY: HYPERX 0x0049af10
// start_pass_upsample

// LIBRARY: HYPERX 0x0049af2a
// sep_upsample

// LIBRARY: HYPERX 0x0049aff7
// fullsize_upsample

// LIBRARY: HYPERX 0x0049b002
// noop_upsample

// LIBRARY: HYPERX 0x0049b00a
// int_upsample

// LIBRARY: HYPERX 0x0049b0f4
// h2v1_upsample

// LIBRARY: HYPERX 0x0049b14b
// h2v2_upsample

// LIBRARY: HYPERX 0x0049b1bd
// h2v1_fancy_upsample

// LIBRARY: HYPERX 0x0049b260
// h2v2_fancy_upsample

// jdcolor.c
// LIBRARY: HYPERX 0x0049b37f
// jinit_color_deconverter

// LIBRARY: HYPERX 0x0049b4a1
// build_ycc_rgb_table

// LIBRARY: HYPERX 0x0049b551
// ycc_rgb_convert

// LIBRARY: HYPERX 0x0049b65f
// null_convert

// LIBRARY: HYPERX 0x0049b6d7
// grayscale_convert

// LIBRARY: HYPERX 0x0049b6fa
// ycck_cmyk_convert

// jdmerge.c
// LIBRARY: HYPERX 0x0049b82e
// jinit_merged_upsampler

// LIBRARY: HYPERX 0x0049b8a4
// build_ycc_rgb_table

// LIBRARY: HYPERX 0x0049b954
// start_pass_merged_upsample

// LIBRARY: HYPERX 0x0049b969
// merged_2v_upsample

// LIBRARY: HYPERX 0x0049ba10
// merged_1v_upsample

// LIBRARY: HYPERX 0x0049ba41
// h2v1_merged_upsample

// LIBRARY: HYPERX 0x0049bb97
// h2v2_merged_upsample

// jquant2.c
// LIBRARY: HYPERX 0x0049bd7e
// jinit_2pass_quantizer

// LIBRARY: HYPERX 0x0049be8d
// init_error_limit

// LIBRARY: HYPERX 0x0049bf2c
// start_pass_2_quant

// LIBRARY: HYPERX 0x0049c02e
// prescan_quantize

// LIBRARY: HYPERX 0x0049c09d
// pass2_no_dither

// LIBRARY: HYPERX 0x0049c14a
// fill_inverse_cmap

// LIBRARY: HYPERX 0x0049c215
// find_nearby_colors

// LIBRARY: HYPERX 0x0049c3b9
// find_best_colors

// LIBRARY: HYPERX 0x0049c4d3
// pass2_fs_dither

// LIBRARY: HYPERX 0x0049c742
// finish_pass1

// LIBRARY: HYPERX 0x0049c764
// select_colors

// LIBRARY: HYPERX 0x0049c7f0
// update_box

// LIBRARY: HYPERX 0x0049cb8a
// median_cut

// LIBRARY: HYPERX 0x0049cc9e
// find_biggest_color_pop

// LIBRARY: HYPERX 0x0049cccd
// find_biggest_volume

// LIBRARY: HYPERX 0x0049ccf6
// compute_color

// LIBRARY: HYPERX 0x0049ce33
// new_color_map_2_quant

// jquant1.c
// LIBRARY: HYPERX 0x0049ce42
// jinit_1pass_quantizer

// LIBRARY: HYPERX 0x0049cecd
// create_colormap

// LIBRARY: HYPERX 0x0049cfff
// select_ncolors

// LIBRARY: HYPERX 0x0049d0d6
// create_colorindex

// LIBRARY: HYPERX 0x0049d202
// alloc_fs_workspace

// LIBRARY: HYPERX 0x0049d23e
// start_pass_1_quant

// LIBRARY: HYPERX 0x0049d30a
// create_odither_tables

// LIBRARY: HYPERX 0x0049d369
// make_odither_array

// LIBRARY: HYPERX 0x0049d3cc
// color_quantize

// LIBRARY: HYPERX 0x0049d457
// color_quantize3

// LIBRARY: HYPERX 0x0049d4e5
// quantize_ord_dither

// LIBRARY: HYPERX 0x0049d5d0
// quantize3_ord_dither

// LIBRARY: HYPERX 0x0049d6d0
// quantize_fs_dither

// LIBRARY: HYPERX 0x0049d854
// new_color_map_1_quant

// jidctred.c
// LIBRARY: HYPERX 0x0049d868
// jpeg_idct_4x4

// LIBRARY: HYPERX 0x0049db88
// jpeg_idct_2x2

// LIBRARY: HYPERX 0x0049dd4b
// jpeg_idct_1x1

// jidctint.c
// LIBRARY: HYPERX 0x0049dd87
// jpeg_idct_islow

// jidctfst.c
// LIBRARY: HYPERX 0x0049e28f
// jpeg_idct_ifast

// jidctflt.c
// LIBRARY: HYPERX 0x0049e687
// jpeg_idct_float

// The library's public tables.
// GLOBAL: HYPERX 0x004b4148
// jpeg_std_message_table

// GLOBAL: HYPERX 0x004b433c
// jpeg_natural_order

// The library's static tables, named in our PDB since the library is built with /Zi.
// GLOBAL: HYPERX 0x004d2c04
// sig_cinfo

// GLOBAL: HYPERX 0x004b432c
// first_pool_slop

// GLOBAL: HYPERX 0x004b4334
// extra_pool_slop

// GLOBAL: HYPERX 0x004b447c
// extend_test

// GLOBAL: HYPERX 0x004b44bc
// extend_offset

// GLOBAL: HYPERX 0x004b44fc
// extend_test

// GLOBAL: HYPERX 0x004b453c
// extend_offset

// GLOBAL: HYPERX 0x004b4580
// aanscales

// GLOBAL: HYPERX 0x004b4600
// aanscalefactor

// GLOBAL: HYPERX 0x004b4640
// base_dither_matrix

// GLOBAL: HYPERX 0x004b4740
// RGB_order

#endif

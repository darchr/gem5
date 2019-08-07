# Verilated -*- Makefile -*-
# DESCRIPTION: Verilator output: Make include file with class lists
#
# This file lists generated Verilated files, for including in higher level makefiles.
# See VNV_nvdla.mk for the caller.

### Switches...
# Coverage output mode?  0/1 (from --coverage)
VM_COVERAGE = 0
# Threaded output mode?  0/1/N threads (from --threads)
VM_THREADS = 0
# Tracing output mode?  0/1 (from --trace)
VM_TRACE = 0

### Object file lists...
# Generated module classes, fast-path, compile with highest optimization
VM_CLASSES_FAST += \
	VNV_nvdla \
	VNV_nvdla_NV_NVDLA_partition_m \
	VNV_nvdla_nv_ram_rws_256x512 \
	VNV_nvdla_NV_NVDLA_CDP_WDMA_dat_fifo \
	VNV_nvdla_NV_NVDLA_RUBIK_fifo \
	VNV_nvdla_nv_ram_rws_32x16 \
	VNV_nvdla_NV_NVDLA_CDMA_dual_reg \
	VNV_nvdla_NV_NVDLA_CDMA_CVT_cell \
	VNV_nvdla_nv_ram_rws_16x256 \
	VNV_nvdla_NV_NVDLA_CSC_pra_cell \
	VNV_nvdla_NV_NVDLA_CMAC_CORE_mac \
	VNV_nvdla_nv_ram_rws_32x768 \
	VNV_nvdla_nv_ram_rws_32x544 \
	VNV_nvdla_NV_NVDLA_CACC_CALC_int16 \
	VNV_nvdla_NV_NVDLA_CACC_CALC_int8 \
	VNV_nvdla_NV_NVDLA_CACC_CALC_fp_48b \
	VNV_nvdla_nv_ram_rws_32x512 \
	VNV_nvdla_NV_NVDLA_SDP_CORE_x \
	VNV_nvdla_NV_NVDLA_SDP_REG_dual \
	VNV_nvdla_NV_NVDLA_MCIF_READ_EG_lat_fifo \
	VNV_nvdla_NV_NVDLA_MCIF_READ_EG_ro_fifo \
	VNV_nvdla_NV_NVDLA_CVIF_READ_EG_lat_fifo \
	VNV_nvdla_NV_NVDLA_CVIF_READ_EG_ro_fifo \
	VNV_nvdla_NV_NVDLA_CDP_RDMA_ro_fifo \
	VNV_nvdla_NV_NVDLA_PDP_RDMA_ro_fifo \
	VNV_nvdla_NV_NVDLA_PDP_WDMA_DAT_fifo \
	VNV_nvdla_NV_NVDLA_MCIF_READ_IG_bpt \
	VNV_nvdla_nv_ram_rws_256x7__F1 \
	VNV_nvdla_nv_ram_rwst_256x8 \
	VNV_nvdla_NV_NVDLA_MCIF_WRITE_IG_bpt \
	VNV_nvdla_nv_ram_rws_256x3__F1 \
	VNV_nvdla_NV_NVDLA_CVIF_READ_IG_bpt \
	VNV_nvdla_NV_NVDLA_CVIF_WRITE_IG_bpt \
	VNV_nvdla_HLS_cdp_icvt \
	VNV_nvdla_HLS_fp17_to_fp32 \
	VNV_nvdla_HLS_fp32_mul \
	VNV_nvdla_fp_sum_block \
	VNV_nvdla_NV_NVDLA_CDP_DP_LUT_CTRL_unit \
	VNV_nvdla_fp_format_cvt \
	VNV_nvdla_NV_NVDLA_CDP_DP_INTP_unit \
	VNV_nvdla_HLS_cdp_ocvt \
	VNV_nvdla_NV_NVDLA_PDP_CORE_unit1d \
	VNV_nvdla_nv_ram_rws_64x116 \
	VNV_nvdla_HLS_fp17_to_fp16 \
	VNV_nvdla_nv_ram_rwsp_128x6__F1 \
	VNV_nvdla_NV_NVDLA_CMAC_CORE_MAC_mul \
	VNV_nvdla_NV_NVDLA_SDP_CORE_Y_cvt \
	VNV_nvdla_RAMPDP_32X288_GL_M1_D2 \
	VNV_nvdla_nv_ram_rwsp_61x514__F1 \
	VNV_nvdla_HLS_fp16_to_fp17 \
	VNV_nvdla_HLS_fp32_add \
	VNV_nvdla_HLS_fp17_mul \
	VNV_nvdla_HLS_fp32_sub \
	VNV_nvdla_nv_ram_rwsp_160x16__F1 \
	VNV_nvdla_NV_NVDLA_SDP_BRDMA_EG_ro \
	VNV_nvdla_NV_NVDLA_SDP_NRDMA_EG_ro \
	VNV_nvdla_NV_NVDLA_SDP_ERDMA_EG_ro \
	VNV_nvdla_HLS_fp17_add \
	VNV_nvdla_nv_ram_rwsp_160x514__F1 \
	VNV_nvdla_nv_ram_rwsp_80x514__F1 \
	VNV_nvdla_ScanShareSel_JTAG_reg_ext_cg__pi27 \
	VNV_nvdla_ScanShareSel_JTAG_reg_ext_cg__pi16 \
	VNV_nvdla_ScanShareSel_JTAG_reg_ext_cg__pi7 \
	VNV_nvdla_ScanShareSel_JTAG_reg_ext_cg__pi3 \
	VNV_nvdla_ScanShareSel_JTAG_reg_ext_cg__pi1 \

# Generated module classes, non-fast-path, compile with low/medium optimization
VM_CLASSES_SLOW += \
	VNV_nvdla__Slow \
	VNV_nvdla_NV_NVDLA_partition_m__Slow \
	VNV_nvdla_nv_ram_rws_256x512__Slow \
	VNV_nvdla_NV_NVDLA_CDP_WDMA_dat_fifo__Slow \
	VNV_nvdla_NV_NVDLA_RUBIK_fifo__Slow \
	VNV_nvdla_nv_ram_rws_32x16__Slow \
	VNV_nvdla_NV_NVDLA_CDMA_dual_reg__Slow \
	VNV_nvdla_NV_NVDLA_CDMA_CVT_cell__Slow \
	VNV_nvdla_nv_ram_rws_16x256__Slow \
	VNV_nvdla_NV_NVDLA_CSC_pra_cell__Slow \
	VNV_nvdla_NV_NVDLA_CMAC_CORE_mac__Slow \
	VNV_nvdla_nv_ram_rws_32x768__Slow \
	VNV_nvdla_nv_ram_rws_32x544__Slow \
	VNV_nvdla_NV_NVDLA_CACC_CALC_int16__Slow \
	VNV_nvdla_NV_NVDLA_CACC_CALC_int8__Slow \
	VNV_nvdla_NV_NVDLA_CACC_CALC_fp_48b__Slow \
	VNV_nvdla_nv_ram_rws_32x512__Slow \
	VNV_nvdla_NV_NVDLA_SDP_CORE_x__Slow \
	VNV_nvdla_NV_NVDLA_SDP_REG_dual__Slow \
	VNV_nvdla_NV_NVDLA_MCIF_READ_EG_lat_fifo__Slow \
	VNV_nvdla_NV_NVDLA_MCIF_READ_EG_ro_fifo__Slow \
	VNV_nvdla_NV_NVDLA_CVIF_READ_EG_lat_fifo__Slow \
	VNV_nvdla_NV_NVDLA_CVIF_READ_EG_ro_fifo__Slow \
	VNV_nvdla_NV_NVDLA_CDP_RDMA_ro_fifo__Slow \
	VNV_nvdla_NV_NVDLA_PDP_RDMA_ro_fifo__Slow \
	VNV_nvdla_NV_NVDLA_PDP_WDMA_DAT_fifo__Slow \
	VNV_nvdla_NV_NVDLA_MCIF_READ_IG_bpt__Slow \
	VNV_nvdla_nv_ram_rws_256x7__F1__Slow \
	VNV_nvdla_nv_ram_rwst_256x8__Slow \
	VNV_nvdla_NV_NVDLA_MCIF_WRITE_IG_bpt__Slow \
	VNV_nvdla_nv_ram_rws_256x3__F1__Slow \
	VNV_nvdla_NV_NVDLA_CVIF_READ_IG_bpt__Slow \
	VNV_nvdla_NV_NVDLA_CVIF_WRITE_IG_bpt__Slow \
	VNV_nvdla_HLS_cdp_icvt__Slow \
	VNV_nvdla_HLS_fp17_to_fp32__Slow \
	VNV_nvdla_HLS_fp32_mul__Slow \
	VNV_nvdla_fp_sum_block__Slow \
	VNV_nvdla_NV_NVDLA_CDP_DP_LUT_CTRL_unit__Slow \
	VNV_nvdla_fp_format_cvt__Slow \
	VNV_nvdla_NV_NVDLA_CDP_DP_INTP_unit__Slow \
	VNV_nvdla_HLS_cdp_ocvt__Slow \
	VNV_nvdla_NV_NVDLA_PDP_CORE_unit1d__Slow \
	VNV_nvdla_nv_ram_rws_64x116__Slow \
	VNV_nvdla_HLS_fp17_to_fp16__Slow \
	VNV_nvdla_nv_ram_rwsp_128x6__F1__Slow \
	VNV_nvdla_NV_NVDLA_CMAC_CORE_MAC_mul__Slow \
	VNV_nvdla_NV_NVDLA_SDP_CORE_Y_cvt__Slow \
	VNV_nvdla_RAMPDP_32X288_GL_M1_D2__Slow \
	VNV_nvdla_nv_ram_rwsp_61x514__F1__Slow \
	VNV_nvdla_HLS_fp16_to_fp17__Slow \
	VNV_nvdla_HLS_fp32_add__Slow \
	VNV_nvdla_HLS_fp17_mul__Slow \
	VNV_nvdla_HLS_fp32_sub__Slow \
	VNV_nvdla_nv_ram_rwsp_160x16__F1__Slow \
	VNV_nvdla_NV_NVDLA_SDP_BRDMA_EG_ro__Slow \
	VNV_nvdla_NV_NVDLA_SDP_NRDMA_EG_ro__Slow \
	VNV_nvdla_NV_NVDLA_SDP_ERDMA_EG_ro__Slow \
	VNV_nvdla_HLS_fp17_add__Slow \
	VNV_nvdla_nv_ram_rwsp_160x514__F1__Slow \
	VNV_nvdla_nv_ram_rwsp_80x514__F1__Slow \
	VNV_nvdla_ScanShareSel_JTAG_reg_ext_cg__pi27__Slow \
	VNV_nvdla_ScanShareSel_JTAG_reg_ext_cg__pi16__Slow \
	VNV_nvdla_ScanShareSel_JTAG_reg_ext_cg__pi7__Slow \
	VNV_nvdla_ScanShareSel_JTAG_reg_ext_cg__pi3__Slow \
	VNV_nvdla_ScanShareSel_JTAG_reg_ext_cg__pi1__Slow \

# Generated support classes, fast-path, compile with highest optimization
VM_SUPPORT_FAST += \

# Generated support classes, non-fast-path, compile with low/medium optimization
VM_SUPPORT_SLOW += \
	VNV_nvdla__Syms \

# Global classes, need linked once per executable, fast-path, compile with highest optimization
VM_GLOBAL_FAST += \
	verilated \

# Global classes, need linked once per executable, non-fast-path, compile with low/medium optimization
VM_GLOBAL_SLOW += \


# Verilated -*- Makefile -*-

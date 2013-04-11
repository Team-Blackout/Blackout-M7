/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <mach/kgsl.h>
#include <mach/msm_bus_board.h>
#include <mach/board.h>
#include <mach/msm_dcvs.h>
#include <mach/socinfo.h>
#include "devices.h"
#include "board-m7.h"
#ifdef CONFIG_CMDLINE_OPTIONS
unsigned int cmdline_2dgpu = CMDLINE_2DGPU_DEFKHZ;
unsigned int cmdline_3dgpu[2] = {CMDLINE_3DGPU_DEFKHZ_0, CMDLINE_3DGPU_DEFKHZ_1};

static int __init devices_read_2dgpu_cmdline(char *khz)
{
	unsigned long ui_khz;
	unsigned long *f;
	unsigned long valid_freq[5] = {200000000, 228571000, 266667000, 300000000, 0};
	int err;

	err = strict_strtoul(khz, 0, &ui_khz);
	if (err) {
		cmdline_2dgpu = CMDLINE_2DGPU_DEFKHZ;
		printk(KERN_INFO "[cmdline_2dgpu]: ERROR while converting! using default value!");
		printk(KERN_INFO "[cmdline_2dgpu]: 2dgpukhz='%i'\n", cmdline_2dgpu);
		return 1;
	}

	/* Check if parsed value is valid */
	if (ui_khz > 300000000)
		cmdline_2dgpu = CMDLINE_2DGPU_DEFKHZ;

	if (ui_khz < 200000000)
		cmdline_2dgpu = CMDLINE_2DGPU_DEFKHZ;

	for (f = valid_freq; f != 0; f++) {
		if (*f == ui_khz) {
			cmdline_2dgpu = ui_khz;
			printk(KERN_INFO "[cmdline_2dgpu]: 2dgpukhz='%u'\n", cmdline_2dgpu);
			return 1;
		}
		if (ui_khz > *f) {
			f++;
			if (ui_khz < *f) {
				f--;
				cmdline_2dgpu = *f;
				printk(KERN_INFO "[cmdline_2dgpu]: AUTOCORRECT! Couldn't find entered value");
				printk(KERN_INFO "[cmdline_2dgpu]: 2dgpukhz='%u'\n", cmdline_2dgpu);
				return 1;
			}
			f--;
		}
	}
	/* if we are still in here then something went wrong. Use defaults */
	cmdline_2dgpu = CMDLINE_2DGPU_DEFKHZ;
	printk(KERN_INFO "[cmdline_2dgpu]: ERROR! using default value!");
	printk(KERN_INFO "[cmdline_2dgpu]: 2dgpukhz='%u'\n", cmdline_2dgpu);
        return 1;
}
__setup("2dgpu=", devices_read_2dgpu_cmdline);

static int __init devices_read_3dgpu_cmdline(char *khz)
{
	unsigned long ui_khz;
	unsigned long *f;
	unsigned long valid_freq[6] = {228571000, 266667000, 320000000, 400000000, 450000000, 0};
	int err;

	err = strict_strtoul(khz, 0, &ui_khz);
	if (err) {
		cmdline_3dgpu[1] = CMDLINE_3DGPU_DEFKHZ_1;
		cmdline_3dgpu[0] = CMDLINE_3DGPU_DEFKHZ_0;
		printk(KERN_INFO "[cmdline_3dgpu]: ERROR while converting! using default value!");
		printk(KERN_INFO "[cmdline_3dgpu]: 3dgpukhz_0='%i' & 3dgpukhz_1='%i'\n",
		       cmdline_3dgpu[0], cmdline_3dgpu[1]);
		return 1;
	}

	/* Check if parsed value is valid */
	if (ui_khz > 450000000)
		cmdline_3dgpu[1] = CMDLINE_3DGPU_DEFKHZ_1;
		cmdline_3dgpu[0] = CMDLINE_3DGPU_DEFKHZ_0;

	if (ui_khz < 228571000)
		cmdline_3dgpu[1] = CMDLINE_3DGPU_DEFKHZ_1;
		cmdline_3dgpu[0] = CMDLINE_3DGPU_DEFKHZ_0;

	for (f = valid_freq; f != 0; f++) {
		if (*f == ui_khz) {
			cmdline_3dgpu[0] = ui_khz;
			if (*f == valid_freq[0]) {
				cmdline_3dgpu[1] = CMDLINE_3DGPU_DEFKHZ_1;
			} else {
				f--;
				cmdline_3dgpu[1] = *f;
				f++;
			}
			printk(KERN_INFO "[cmdline_3dgpu]: 3dgpukhz_0='%i' & 3dgpukhz_1='%i'\n",
			       cmdline_3dgpu[0], cmdline_3dgpu[1]);
			return 1;
		}
		if (ui_khz > *f) {
			f++;
			if (ui_khz < *f) {
				f--;
				cmdline_3dgpu[0] = *f;
				if (*f == valid_freq[0]) {
					cmdline_3dgpu[1] = CMDLINE_3DGPU_DEFKHZ_1;
				} else {
					f--;
					cmdline_3dgpu[1] = *f;
					f++;
				}
				printk(KERN_INFO "[cmdline_3dgpu]: AUTOCORRECT! Couldn't find entered value");
				printk(KERN_INFO "[cmdline_3dgpu]: 3dgpukhz_0='%i' & 3dgpukhz_1='%i'\n",
				       cmdline_3dgpu[0], cmdline_3dgpu[1]);
				return 1;
			}
			f--;
		}
	}
	/* if we are still in here then something went wrong. Use defaults */
	cmdline_3dgpu[1] = CMDLINE_3DGPU_DEFKHZ_1;
	cmdline_3dgpu[0] = CMDLINE_3DGPU_DEFKHZ_0;
	printk(KERN_INFO "[cmdline_3dgpu]: ERROR! using default value!");
	printk(KERN_INFO "[cmdline_3dgpu]: 3dgpukhz_0='%i' & 3dgpukhz_1='%i'\n",
	       cmdline_3dgpu[0], cmdline_3dgpu[1]);
        return 1;
}
__setup("3dgpu=", devices_read_3dgpu_cmdline);

#endif
/* end cmdline_gpu */
#ifdef CONFIG_MSM_DCVS
static struct msm_dcvs_freq_entry grp3d_freq[] = {
	{0, 900, 0, 0, 0},
	{0, 950, 0, 0, 0},
	{0, 950, 0, 0, 0},
	{0, 1200, 1, 100, 100},
};


static struct msm_dcvs_freq_entry grp2d_freq[] = {
	{0, 0, 86000},
	{0, 0, 200000},
};

static struct msm_dcvs_core_info grp2d_core_info = {
	.freq_tbl = &grp2d_freq[0],
	.core_param = {
		.max_time_us = 100000,
		.num_freq = ARRAY_SIZE(grp2d_freq),
	},
	.algo_param = {
		.slack_time_us = 39000,
		.disable_pc_threshold = 90000,
		.ss_window_size = 1000000,
		.ss_util_pct = 90,
		.em_max_util_pct = 95,
	},
};

static struct msm_dcvs_core_info grp3d_core_info = {
	.freq_tbl		= &grp3d_freq[0],
	.num_cores		= 1,
	.sensors		= (int[]){0},
	.thermal_poll_ms	= 60000,
	.core_param		= {
		.core_type	= MSM_DCVS_CORE_TYPE_GPU,
	},
	.algo_param		= {
		.disable_pc_threshold	= 0,
		.em_win_size_min_us	= 100000,
		.em_win_size_max_us	= 300000,
		.em_max_util_pct	= 97,
		.group_id		= 0,
		.max_freq_chg_time_us	= 100000,
		.slack_mode_dynamic	= 0,
		.slack_time_min_us	= 39000,
		.slack_time_max_us	= 39000,
		.ss_win_size_min_us	= 1000000,
		.ss_win_size_max_us	= 1000000,
		.ss_util_pct		= 95,
		.ss_no_corr_below_freq	= 0,
	},

	.energy_coeffs		= {
		.leakage_coeff_a	= -17720,
		.leakage_coeff_b	= 37,
		.leakage_coeff_c	= 3329,
		.leakage_coeff_d	= -277,

		.active_coeff_a		= 2492,
		.active_coeff_b		= 0,
		.active_coeff_c		= 0
	},

	.power_param		= {
		.current_temp	= 25,
		.num_freq	= ARRAY_SIZE(grp3d_freq),
	}
};
#endif 

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors grp3d_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_3D,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
	{
		.src = MSM_BUS_MASTER_GRAPHICS_3D_PORT1,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors grp3d_low_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_3D,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(1000),
	},
	{
		.src = MSM_BUS_MASTER_GRAPHICS_3D_PORT1,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(1000),
	},
};

static struct msm_bus_vectors grp3d_nominal_low_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_3D,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(2000),
	},
	{
		.src = MSM_BUS_MASTER_GRAPHICS_3D_PORT1,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(2000),
	},
};

static struct msm_bus_vectors grp3d_nominal_high_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_3D,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(4264),
	},
	{
		.src = MSM_BUS_MASTER_GRAPHICS_3D_PORT1,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(4264),
	},
};

static struct msm_bus_vectors grp3d_max_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_3D,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(5290),
	},
	{
		.src = MSM_BUS_MASTER_GRAPHICS_3D_PORT1,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(5290),
	},
};

static struct msm_bus_paths grp3d_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(grp3d_init_vectors),
		grp3d_init_vectors,
	},
	{
		ARRAY_SIZE(grp3d_low_vectors),
		grp3d_low_vectors,
	},
	{
		ARRAY_SIZE(grp3d_nominal_low_vectors),
		grp3d_nominal_low_vectors,
	},
	{
		ARRAY_SIZE(grp3d_nominal_high_vectors),
		grp3d_nominal_high_vectors,
	},
	{
		ARRAY_SIZE(grp3d_max_vectors),
		grp3d_max_vectors,
	},
};

static struct msm_bus_scale_pdata grp3d_bus_scale_pdata = {
	grp3d_bus_scale_usecases,
	ARRAY_SIZE(grp3d_bus_scale_usecases),
	.name = "grp3d",
};
static struct msm_bus_vectors grp2d0_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_2D_CORE0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors grp2d0_nominal_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_2D_CORE0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(1000),
	},
};

static struct msm_bus_vectors grp2d0_max_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_2D_CORE0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(2048),
	},
};

static struct msm_bus_paths grp2d0_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(grp2d0_init_vectors),
		grp2d0_init_vectors,
	},
	{
		ARRAY_SIZE(grp2d0_nominal_vectors),
		grp2d0_nominal_vectors,
	},
	{
		ARRAY_SIZE(grp2d0_max_vectors),
		grp2d0_max_vectors,
	},
};

struct msm_bus_scale_pdata grp2d0_bus_scale_pdata = {
	grp2d0_bus_scale_usecases,
	ARRAY_SIZE(grp2d0_bus_scale_usecases),
	.name = "grp2d0",
};

static struct msm_bus_vectors grp2d1_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_2D_CORE1,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors grp2d1_nominal_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_2D_CORE1,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(1000),
	},
};

static struct msm_bus_vectors grp2d1_max_vectors[] = {
	{
		.src = MSM_BUS_MASTER_GRAPHICS_2D_CORE1,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = KGSL_CONVERT_TO_MBPS(2048),
	},
};

static struct msm_bus_paths grp2d1_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(grp2d1_init_vectors),
		grp2d1_init_vectors,
	},
	{
		ARRAY_SIZE(grp2d1_nominal_vectors),
		grp2d1_nominal_vectors,
	},
	{
		ARRAY_SIZE(grp2d1_max_vectors),
		grp2d1_max_vectors,
	},
};

struct msm_bus_scale_pdata grp2d1_bus_scale_pdata = {
	grp2d1_bus_scale_usecases,
	ARRAY_SIZE(grp2d1_bus_scale_usecases),
	.name = "grp2d1",
};
#endif

static struct resource kgsl_3d0_resources[] = {
	{
		.name = KGSL_3D0_REG_MEMORY,
		.start = 0x04300000, 
		.end = 0x0431ffff,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = KGSL_3D0_IRQ,
		.start = GFX3D_IRQ,
		.end = GFX3D_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static const struct kgsl_iommu_ctx kgsl_3d0_iommu0_ctxs[] = {
	{ "gfx3d_user", 0 },
	{ "gfx3d_priv", 1 },
};


static const struct kgsl_iommu_ctx kgsl_3d0_iommu1_ctxs[] = {
	{ "gfx3d1_user", 0 },
	{ "gfx3d1_priv", 1 },
};

static struct kgsl_device_iommu_data kgsl_3d0_iommu_data[] = {
	{
		.iommu_ctxs = kgsl_3d0_iommu0_ctxs,
		.iommu_ctx_count = ARRAY_SIZE(kgsl_3d0_iommu0_ctxs),
		.physstart = 0x07C00000,
		.physend = 0x07C00000 + SZ_1M - 1,
	},
	{
		.iommu_ctxs = kgsl_3d0_iommu1_ctxs,
		.iommu_ctx_count = ARRAY_SIZE(kgsl_3d0_iommu1_ctxs),
		.physstart = 0x07D00000,
		.physend = 0x07D00000 + SZ_1M - 1,
	},
};

static struct kgsl_device_platform_data kgsl_3d0_pdata = {
	.pwrlevel = {
		{
			.gpu_freq = 450000000,
			.bus_freq = 5,
			.io_fraction = 0,
		},
        {
			.gpu_freq = 400000000,
			.bus_freq = 4,
			.io_fraction = 0,
		},
		{
			.gpu_freq = 320000000,
			.bus_freq = 3,
			.io_fraction = 33,
		},
		{
			.gpu_freq = 200000000,
			.bus_freq = 2,
			.io_fraction = 100,
		},
		{
			.gpu_freq = 27000000,
			.bus_freq = 0,
		},
	},
	.init_level = 2,
	.num_levels = 5,
	.set_grp_async = NULL,
	.idle_timeout = HZ/10,
	.nap_allowed = true,
	.strtstp_sleepwake = false,
	.clk_map = KGSL_CLK_CORE | KGSL_CLK_IFACE | KGSL_CLK_MEM_IFACE,
#ifdef CONFIG_MSM_BUS_SCALING
	.bus_scale_table = &grp3d_bus_scale_pdata,
#endif
	.iommu_data = kgsl_3d0_iommu_data,
	.iommu_count = ARRAY_SIZE(kgsl_3d0_iommu_data),
#ifdef CONFIG_MSM_DCVS
	.core_info = &grp3d_core_info,
#endif
};

static struct platform_device device_kgsl_3d0 = {
	.name = "kgsl-3d0",
	.id = 0,
	.num_resources = ARRAY_SIZE(kgsl_3d0_resources),
	.resource = kgsl_3d0_resources,
	.dev = {
		.platform_data = &kgsl_3d0_pdata,
	},
};
static struct resource kgsl_2d0_resources[] = {
	{
		.name = KGSL_2D0_REG_MEMORY,
		.start = 0x04100000, 
		.end = 0x04100FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = KGSL_2D0_IRQ,
		.start = GFX2D0_IRQ,
		.end = GFX2D0_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static const struct kgsl_iommu_ctx kgsl_2d0_iommu_ctxs[] = {
	{ "gfx2d0_2d0", 0 },
};

static struct kgsl_device_iommu_data kgsl_2d0_iommu_data[] = {
	{
		.iommu_ctxs = kgsl_2d0_iommu_ctxs,
		.iommu_ctx_count = ARRAY_SIZE(kgsl_2d0_iommu_ctxs),
		.physstart = 0x07D00000,
		.physend = 0x07D00000 + SZ_1M - 1,
	},
};

static struct kgsl_device_platform_data kgsl_2d0_pdata = {
	.pwrlevel = {
		{
			  .gpu_freq = 300000000,
			   .bus_freq = 4,
		},
		{
			.gpu_freq = 266667000,
			.bus_freq = 3,
		},
		{
			.gpu_freq = 200000000,
			.bus_freq = 2,
		},
		{
			.gpu_freq = 96000000,
			.bus_freq = 1,
		},
		{
			.gpu_freq = 27000000,
			.bus_freq = 0,
		},
	},
	.init_level = 0,
	.num_levels = 5,
	.set_grp_async = NULL,
	.idle_timeout = HZ/10,
	.nap_allowed = true,
	.clk_map = KGSL_CLK_CORE | KGSL_CLK_IFACE,
#ifdef CONFIG_MSM_BUS_SCALING
	.bus_scale_table = &grp2d0_bus_scale_pdata,
#endif
	.iommu_data = kgsl_2d0_iommu_data,
	.iommu_count = ARRAY_SIZE(kgsl_2d0_iommu_data),
	.core_info = &grp2d_core_info,
};

struct platform_device msm_kgsl_2d0 = {
	.name = "kgsl-2d0",
	.id = 0,
	.num_resources = ARRAY_SIZE(kgsl_2d0_resources),
	.resource = kgsl_2d0_resources,
	.dev = {
		.platform_data = &kgsl_2d0_pdata,
	},
};

static const struct kgsl_iommu_ctx kgsl_2d1_iommu_ctxs[] = {
	{ "gfx2d1_2d1", 0 },
};

static struct kgsl_device_iommu_data kgsl_2d1_iommu_data[] = {
	{
		.iommu_ctxs = kgsl_2d1_iommu_ctxs,
		.iommu_ctx_count = ARRAY_SIZE(kgsl_2d1_iommu_ctxs),
		.physstart = 0x07E00000,
		.physend = 0x07E00000 + SZ_1M - 1,
	},
};

static struct resource kgsl_2d1_resources[] = {
	{
		.name = KGSL_2D1_REG_MEMORY,
		.start = 0x04200000, 
		.end =   0x04200FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = KGSL_2D1_IRQ,
		.start = GFX2D1_IRQ,
		.end = GFX2D1_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct kgsl_device_platform_data kgsl_2d1_pdata = {
	.pwrlevel = {
		{
			  .gpu_freq = 300000000,
			   .bus_freq = 4,
		},
		{
			.gpu_freq = 266667000,
			.bus_freq = 3,
		},
		{
			.gpu_freq = 200000000,
			.bus_freq = 2,
		},
		{
			.gpu_freq = 96000000,
			.bus_freq = 1,
		},
		{
			.gpu_freq = 27000000,
			.bus_freq = 0,
		},
	},
	.init_level = 0,
	.num_levels = 5,
	.set_grp_async = NULL,
	.idle_timeout = HZ/10,
	.nap_allowed = true,
	.clk_map = KGSL_CLK_CORE | KGSL_CLK_IFACE,
#ifdef CONFIG_MSM_BUS_SCALING
	.bus_scale_table = &grp2d1_bus_scale_pdata,
#endif
	.iommu_data = kgsl_2d1_iommu_data,
	.iommu_count = ARRAY_SIZE(kgsl_2d1_iommu_data),
	.core_info = &grp2d_core_info,
};

struct platform_device msm_kgsl_2d1 = {
	.name = "kgsl-2d1",
	.id = 1,
	.num_resources = ARRAY_SIZE(kgsl_2d1_resources),
	.resource = kgsl_2d1_resources,
	.dev = {
		.platform_data = &kgsl_2d1_pdata,
	},
};
#ifdef CONFIG_CMDLINE_OPTIONS
/* setters for cmdline_gpu */
extern int set_kgsl_3d0_freq(unsigned int freq0, unsigned int freq1)
{
	kgsl_3d0_pdata.pwrlevel[0].gpu_freq = freq0;
	kgsl_3d0_pdata.pwrlevel[1].gpu_freq = freq1;
	return 0;
}
extern int set_kgsl_2d0_freq(unsigned int freq)
{
	kgsl_2d0_pdata.pwrlevel[0].gpu_freq = freq;
	return 0;
}
extern int set_kgsl_2d1_freq(unsigned int freq)
{
	kgsl_2d1_pdata.pwrlevel[0].gpu_freq = freq;
	return 0;
}
#endif
void __init m7_init_gpu(void)
{
	unsigned int version = socinfo_get_version();

	if (cpu_is_apq8064ab())
		kgsl_3d0_pdata.pwrlevel[0].gpu_freq = 400000000;
	if (SOCINFO_VERSION_MAJOR(version) == 2) {
		kgsl_3d0_pdata.chipid = ADRENO_CHIPID(3, 2, 0, 2);
	} else {
		if ((SOCINFO_VERSION_MAJOR(version) == 1) &&
				(SOCINFO_VERSION_MINOR(version) == 1))
			kgsl_3d0_pdata.chipid = ADRENO_CHIPID(3, 2, 0, 1);
		else
			kgsl_3d0_pdata.chipid = ADRENO_CHIPID(3, 2, 0, 0);
	}

	platform_device_register(&device_kgsl_3d0);
}

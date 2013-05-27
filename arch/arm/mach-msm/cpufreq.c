/* arch/arm/mach-msm/cpufreq.c
 *
 * MSM architecture cpufreq driver
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2007-2013, The Linux Foundation. All rights reserved.
 * Author: Mike A. Chan <mikechan@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <linux/earlysuspend.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/suspend.h>
#include <mach/socinfo.h>
#include <mach/cpufreq.h>

#include "acpuclock.h"

#ifdef CONFIG_SMP
struct cpufreq_work_struct {
	struct work_struct work;
	struct cpufreq_policy *policy;
	struct completion complete;
	int frequency;
	int status;
};

static DEFINE_PER_CPU(struct cpufreq_work_struct, cpufreq_work);
static struct workqueue_struct *msm_cpufreq_wq;
#endif

struct cpufreq_suspend_t {
	struct mutex suspend_mutex;
	int device_suspended;
};

static DEFINE_PER_CPU(struct cpufreq_suspend_t, cpufreq_suspend);

struct cpu_freq {
	uint32_t max;
	uint32_t min;
	uint32_t allowed_max;
	uint32_t allowed_min;
	uint32_t limits_init;
	
};

static DEFINE_PER_CPU(struct cpu_freq, cpu_freq_info);
static int override_cpu;
#ifdef CONFIG_CMDLINE_OPTIONS
/*
 * start cmdline_khz
 */

/* to be safe, fill vars with defaults */


uint32_t cmdline_maxkhz = CONFIG_MSM_CPU_FREQ_MAX, cmdline_minkhz = CONFIG_MSM_CPU_FREQ_MIN;

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ASSWAX
char cmdline_gov[16] = "AssWax";
#endif
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_BADASS
char cmdline_gov[16] = "Badass";
#endif
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_USERSPACE
char cmdline_gov[16] = "userspace";
#endif
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_POWERSAVE
char cmdline_gov[16] = "powersave";
#endif
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_LIONHEART
char cmdline_gov[16] = "lionheart";
#endif
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
char cmdline_gov[16] = "ondemand";
#endif
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_PERFORMANCE
char cmdline_gov[16] = "performance";
#endif
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_INTELLIDEMAND
char cmdline_gov[16] = "intellidemand";
#endif
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_SMARTMAX
char cmdline_gov[16] = "smartmax";
#endif

uint32_t cmdline_maxscroff = 486000;
bool cmdline_scroff = false;

/* only override the governor 2 times, when
 * initially bringing up cpufreq on the cpus */
int cmdline_gov_cnt = CONFIG_NR_CPUS;

static int __init cpufreq_read_maxkhz_cmdline(char *maxkhz)
{
	uint32_t check;
	unsigned long ui_khz;
	int err;

	err = strict_strtoul(maxkhz, 0, &ui_khz);
	if (err) {
		cmdline_maxkhz = CONFIG_MSM_CPU_FREQ_MAX;
		printk(KERN_INFO "[cmdline_khz_max]: ERROR while converting! using default value!");
		printk(KERN_INFO "[cmdline_khz_max]: maxkhz='%i'\n", cmdline_maxkhz);
		return 1;
	}

	check = acpu_check_khz_value(ui_khz);

	if (check == 1) {
		cmdline_maxkhz = ui_khz;
		printk(KERN_INFO "[cmdline_khz_max]: maxkhz='%u'\n", cmdline_maxkhz);
	}
	if (check == 0) {
		cmdline_maxkhz = CONFIG_MSM_CPU_FREQ_MAX;
		printk(KERN_INFO "[cmdline_khz_max]: ERROR! using default value!");
		printk(KERN_INFO "[cmdline_khz_max]: maxkhz='%u'\n", cmdline_maxkhz);
	}
	if (check > 1) {
		cmdline_maxkhz = check;
		printk(KERN_INFO "[cmdline_khz_max]: AUTOCORRECT! Could not find entered value in the acpu table!");
		printk(KERN_INFO "[cmdline_khz_max]: maxkhz='%u'\n", cmdline_maxkhz);
	}
        return 1;
}
__setup("maxkhz=", cpufreq_read_maxkhz_cmdline);

static int __init cpufreq_read_minkhz_cmdline(char *minkhz)
{
	uint32_t check;
	unsigned long ui_khz;
	int err;

	err = strict_strtoul(minkhz, 0, &ui_khz);
	if (err) {
		cmdline_minkhz = CONFIG_MSM_CPU_FREQ_MIN;
		printk(KERN_INFO "[cmdline_khz_min]: ERROR while converting! using default value!");
		printk(KERN_INFO "[cmdline_khz_min]: minkhz='%i'\n", cmdline_minkhz);
		return 1;
	}

	check = acpu_check_khz_value(ui_khz);

	if (check == 1) {
		cmdline_minkhz = ui_khz;
		printk(KERN_INFO "[cmdline_khz_min]: minkhz='%u'\n", cmdline_minkhz);
	}
	if (check == 0) {
		cmdline_minkhz = CONFIG_MSM_CPU_FREQ_MIN;
		printk(KERN_INFO "[cmdline_khz_min]: ERROR! using default value!");
		printk(KERN_INFO "[cmdline_khz_min]: minkhz='%u'\n", cmdline_minkhz);
	}
	if (check > 1) {
		cmdline_minkhz = check;
		printk(KERN_INFO "[cmdline_khz_min]: AUTOCORRECT! Could not find entered value in the acpu table!");
		printk(KERN_INFO "[cmdline_khz_min]: minkhz='%u'\n", cmdline_minkhz);
	}
        return 1;
}
__setup("minkhz=", cpufreq_read_minkhz_cmdline);

static int __init cpufreq_read_gov_cmdline(char *gov)
{
	if (gov) {
		strcpy(cmdline_gov, gov);
		printk(KERN_INFO "[cmdline_gov]: Governor will be set to '%s'", cmdline_gov);
	} else {
		printk(KERN_INFO "[cmdline_gov]: No input found.");
	}
	return 1;
}
__setup("gov=", cpufreq_read_gov_cmdline);

static int __init cpufreq_read_maxscroff_cmdline(char *maxscroff)
{
	uint32_t check;
	unsigned long ui_khz;
	int err;

	err = strict_strtoul(maxscroff, 0, &ui_khz);
	if (err) {
		cmdline_maxscroff = cmdline_maxkhz;
		printk(KERN_INFO "[cmdline_maxscroff]: ERROR while converting! using maxkhz value!");
		printk(KERN_INFO "[cmdline_maxscroff]: maxscroff='%i'\n", cmdline_maxscroff);
		return 1;
	}

	check = acpu_check_khz_value(ui_khz);

	if (check == 1) {
		cmdline_maxscroff = ui_khz;
		printk(KERN_INFO "[cmdline_maxscroff]: maxscroff='%u'\n", cmdline_maxscroff);
	}
	if (check == 0) {
		cmdline_maxscroff = cmdline_maxkhz;
		printk(KERN_INFO "[cmdline_maxscroff]: ERROR! using maxkhz value!");
		printk(KERN_INFO "[cmdline_maxscroff]: maxscroff='%u'\n", cmdline_maxscroff);
	}
	if (check > 1) {
		cmdline_maxscroff = check;
		printk(KERN_INFO "[cmdline_maxscroff]: AUTOCORRECT! Could not find entered value in the acpu table!");
		printk(KERN_INFO "[cmdline_maxscroff]: maxscroff='%u'\n", cmdline_maxscroff);
	}
        return 1;
}
__setup("maxscroff=", cpufreq_read_maxscroff_cmdline);
/* end cmdline_khz */
#endif
static int set_cpu_freq(struct cpufreq_policy *policy, unsigned int new_freq)
{
	int ret = 0;
#ifdef CONFIG_PERFLOCK
	int perf_freq = 0;
#endif
	struct cpufreq_freqs freqs;
	struct cpu_freq *limit = &per_cpu(cpu_freq_info, policy->cpu);

	freqs.old = policy->cur;
#ifdef CONFIG_PERFLOCK
	perf_freq = perflock_override(policy, new_freq);
	if (perf_freq) {
		if (policy->cur == perf_freq)
			return 0;
		else
			freqs.new = perf_freq;
	} else if (override_cpu) {
#else
	if (override_cpu) {
#endif
		if (policy->cur == policy->max)
			return 0;
		else
			freqs.new = policy->max;
	} else
		freqs.new = new_freq;

	if (limit->limits_init) {
		if (freqs.new > limit->allowed_max) {
			freqs.new = limit->allowed_max;
			pr_debug("max: limiting freq to %d\n", new_freq);
		}

		if (freqs.new < limit->allowed_min) {
			freqs.new = limit->allowed_min;
			pr_debug("min: limiting freq to %d\n", new_freq);
		}
	}

	freqs.cpu = policy->cpu;
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	ret = acpuclk_set_rate(policy->cpu, freqs.new, SETRATE_CPUFREQ);
	if (!ret)
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	return ret;
}

#ifdef CONFIG_SMP
static void set_cpu_work(struct work_struct *work)
{
	struct cpufreq_work_struct *cpu_work =
		container_of(work, struct cpufreq_work_struct, work);

	cpu_work->status = set_cpu_freq(cpu_work->policy, cpu_work->frequency);
	complete(&cpu_work->complete);
}
#endif
#ifdef CONFIG_CMDLINE_OPTIONS
static void msm_cpufreq_early_suspend(struct early_suspend *h)
{
	uint32_t curfreq;
	int cpu;

	for_each_possible_cpu(cpu) {
		mutex_lock(&per_cpu(cpufreq_suspend, cpu).suspend_mutex);
		if (cmdline_maxscroff) {
			cmdline_scroff = true;
			curfreq = acpuclk_get_rate(cpu);
			if (curfreq > cmdline_maxscroff) {
				acpuclk_set_rate(cpu, cmdline_maxscroff, SETRATE_CPUFREQ);
				curfreq = acpuclk_get_rate(cpu);
				printk(KERN_INFO "[cmdline - Freq limiter]: Limited freq to '%u'\n", curfreq);
			}
		}
		mutex_unlock(&per_cpu(cpufreq_suspend, cpu).suspend_mutex);
	}
}

static void msm_cpufreq_late_resume(struct early_suspend *h)
{
	uint32_t curfreq;
	int cpu;
	struct cpufreq_work_struct *cpu_work;

	for_each_possible_cpu(cpu) {
		mutex_lock(&per_cpu(cpufreq_suspend, cpu).suspend_mutex);
		if (cmdline_scroff == true) {
			cmdline_scroff = false;
			cpu_work = &per_cpu(cpufreq_work, cpu);
			curfreq = acpuclk_get_rate(cpu);
			if (curfreq != cpu_work->frequency) {
				acpuclk_set_rate(cpu, cmdline_maxkhz, SETRATE_CPUFREQ);
				curfreq = acpuclk_get_rate(cpu);
				printk(KERN_INFO "[cmdline - Freq limiter]: Unlocking freq to '%u'\n", curfreq);
			}
		}
		mutex_unlock(&per_cpu(cpufreq_suspend, cpu).suspend_mutex);
	}
}

static struct early_suspend msm_cpufreq_early_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 10,
	.suspend = msm_cpufreq_early_suspend,
	.resume = msm_cpufreq_late_resume,
};
#endif
static int msm_cpufreq_target(struct cpufreq_policy *policy,
				unsigned int target_freq,
				unsigned int relation)
{
	int ret = -EFAULT;
	int index;
	struct cpufreq_frequency_table *table;
#ifdef CONFIG_SMP
	struct cpufreq_work_struct *cpu_work = NULL;
	cpumask_var_t mask;

	if (!cpu_active(policy->cpu)) {
		pr_info("cpufreq: cpu %d is not active.\n", policy->cpu);
		return -ENODEV;
	}

	if (!alloc_cpumask_var(&mask, GFP_KERNEL))
		return -ENOMEM;
#endif

	mutex_lock(&per_cpu(cpufreq_suspend, policy->cpu).suspend_mutex);

	if (per_cpu(cpufreq_suspend, policy->cpu).device_suspended) {
		pr_debug("cpufreq: cpu%d scheduling frequency change "
				"in suspend.\n", policy->cpu);
		ret = -EFAULT;
		goto done;
	}

	table = cpufreq_frequency_get_table(policy->cpu);
	if (cpufreq_frequency_table_target(policy, table, target_freq, relation,
			&index)) {
		pr_err("cpufreq: invalid target_freq: %d\n", target_freq);
		ret = -EINVAL;
		goto done;
	}

#ifdef CONFIG_CPU_FREQ_DEBUG
	pr_debug("CPU[%d] target %d relation %d (%d-%d) selected %d\n",
		policy->cpu, target_freq, relation,
		policy->min, policy->max, table[index].frequency);
#endif

#ifdef CONFIG_SMP
	cpu_work = &per_cpu(cpufreq_work, policy->cpu);
	cpu_work->policy = policy;
	cpu_work->frequency = table[index].frequency;
	cpu_work->status = -ENODEV;

	cpumask_clear(mask);
	cpumask_set_cpu(policy->cpu, mask);
	if (cpumask_equal(mask, &current->cpus_allowed)) {
		ret = set_cpu_freq(cpu_work->policy, cpu_work->frequency);
		goto done;
	} else {
		cancel_work_sync(&cpu_work->work);
		INIT_COMPLETION(cpu_work->complete);
		queue_work_on(policy->cpu, msm_cpufreq_wq, &cpu_work->work);
		wait_for_completion(&cpu_work->complete);
	}

	ret = cpu_work->status;
#else
	ret = set_cpu_freq(policy, table[index].frequency);
#endif

done:
#ifdef CONFIG_SMP
	free_cpumask_var(mask);
#endif
	mutex_unlock(&per_cpu(cpufreq_suspend, policy->cpu).suspend_mutex);
	return ret;
}

static int msm_cpufreq_verify(struct cpufreq_policy *policy)
{
	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
			policy->cpuinfo.max_freq);
	return 0;
}

static unsigned int msm_cpufreq_get_freq(unsigned int cpu)
{
	return acpuclk_get_rate(cpu);
}

static inline int msm_cpufreq_limits_init(void)
{
	int cpu = 0;
	int i = 0;
	struct cpufreq_frequency_table *table = NULL;
	uint32_t min = (uint32_t) -1;
	uint32_t max = 0;
	struct cpu_freq *limit = NULL;

	for_each_possible_cpu(cpu) {
		limit = &per_cpu(cpu_freq_info, cpu);
		table = cpufreq_frequency_get_table(cpu);
		if (table == NULL) {
			pr_err("%s: error reading cpufreq table for cpu %d\n",
					__func__, cpu);
			continue;
		}
		for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++) {
			if (table[i].frequency > max)
				max = table[i].frequency;
			if (table[i].frequency < min)
				min = table[i].frequency;
		}
		limit->allowed_min = min;
		limit->allowed_max = max;
		limit->min = min;
		limit->max = max;
		limit->limits_init = 1;
	}

	return 0;
}

int msm_cpufreq_set_freq_limits(uint32_t cpu, uint32_t min, uint32_t max)
{
	struct cpu_freq *limit = &per_cpu(cpu_freq_info, cpu);

	if (!limit->limits_init)
		msm_cpufreq_limits_init();

	if ((min != MSM_CPUFREQ_NO_LIMIT) &&
		min >= limit->min && min <= limit->max)
		limit->allowed_min = min;
	else
		limit->allowed_min = limit->min;


	if ((max != MSM_CPUFREQ_NO_LIMIT) &&
		max <= limit->max && max >= limit->min)
		limit->allowed_max = max;
	else
		limit->allowed_max = limit->max;

	pr_debug("%s: Limiting cpu %d min = %d, max = %d\n",
			__func__, cpu,
			limit->allowed_min, limit->allowed_max);

	return 0;
}
EXPORT_SYMBOL(msm_cpufreq_set_freq_limits);

static int __cpuinit msm_cpufreq_init(struct cpufreq_policy *policy)
{
	int cur_freq;
	int index;
	int ret = 0;
	struct cpufreq_frequency_table *table;
#ifdef CONFIG_SMP
	struct cpufreq_work_struct *cpu_work = NULL;
#endif


	table = cpufreq_frequency_get_table(policy->cpu);
	if (table == NULL)
		return -ENODEV;
	/*
	 * In 8625 both cpu core's frequency can not
	 * be changed independently. Each cpu is bound to
	 * same frequency. Hence set the cpumask to all cpu.
	 */
	if (cpu_is_msm8625())
		cpumask_setall(policy->cpus);

	if (cpufreq_frequency_table_cpuinfo(policy, table)) {
#ifdef CONFIG_MIN_MAX
		policy->cpuinfo.min_freq = CONFIG_MSM_CPU_FREQ_MIN;
		policy->cpuinfo.max_freq = CONFIG_MSM_CPU_FREQ_MAX;
#endif
	}
#ifdef CONFIG_MIN_MAX


	policy->min = CONFIG_MSM_CPU_FREQ_MIN;
	policy->max = CONFIG_MSM_CPU_FREQ_MAX;
#endif

	cur_freq = acpuclk_get_rate(policy->cpu);
	if (cpufreq_frequency_table_target(policy, table, cur_freq,
	    CPUFREQ_RELATION_H, &index) &&
	    cpufreq_frequency_table_target(policy, table, cur_freq,
	    CPUFREQ_RELATION_L, &index)) {
		pr_info("cpufreq: cpu%d at invalid freq: %d\n",
				policy->cpu, cur_freq);
		return -EINVAL;
	}

	ret = set_cpu_freq(policy, table[index].frequency);
	if (ret)
	  return ret;
	pr_debug("cpufreq: cpu%d init at %d switching to %d\n",
	    policy->cpu, cur_freq, table[index].frequency);
	policy->cur = table[index].frequency;

	policy->cpuinfo.transition_latency =
		acpuclk_get_switch_time() * NSEC_PER_USEC;
#ifdef CONFIG_SMP
	cpu_work = &per_cpu(cpufreq_work, policy->cpu);
	INIT_WORK(&cpu_work->work, set_cpu_work);
	init_completion(&cpu_work->complete);
#endif
#ifdef CONFIG_CMDLINE_OPTIONS
	policy->max = cmdline_maxkhz;
	policy->min = cmdline_minkhz;
#else 
	policy->max = CONFIG_MSM_CPU_FREQ_MAX;
	policy->min = CONFIG_MSM_CPU_FREQ_MIN;
#endif
    
	return 0;
}

static int msm_cpufreq_suspend(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		mutex_lock(&per_cpu(cpufreq_suspend, cpu).suspend_mutex);
		per_cpu(cpufreq_suspend, cpu).device_suspended = 1;
		mutex_unlock(&per_cpu(cpufreq_suspend, cpu).suspend_mutex);
	}

	return NOTIFY_DONE;
}

static int msm_cpufreq_resume(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		per_cpu(cpufreq_suspend, cpu).device_suspended = 0;
	}

	return NOTIFY_DONE;
}

static int msm_cpufreq_pm_event(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		return msm_cpufreq_resume();
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		return msm_cpufreq_suspend();
	default:
		return NOTIFY_DONE;
	}
}

static struct freq_attr *msm_freq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver msm_cpufreq_driver = {
	/* lps calculations are handled here. */
	.flags		= CPUFREQ_STICKY | CPUFREQ_CONST_LOOPS,
	.init		= msm_cpufreq_init,
	.verify		= msm_cpufreq_verify,
	.target		= msm_cpufreq_target,
	.get		= msm_cpufreq_get_freq,
	.name		= "msm",
	.attr		= msm_freq_attr,
};

static struct notifier_block msm_cpufreq_pm_notifier = {
	.notifier_call = msm_cpufreq_pm_event,
};

static int __init msm_cpufreq_register(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		mutex_init(&(per_cpu(cpufreq_suspend, cpu).suspend_mutex));
		per_cpu(cpufreq_suspend, cpu).device_suspended = 0;
	}

#ifdef CONFIG_SMP
	msm_cpufreq_wq = create_workqueue("msm-cpufreq");
#endif

	register_pm_notifier(&msm_cpufreq_pm_notifier);
#ifdef CONFIG_CMDLINE_OPTIONS
	register_early_suspend(&msm_cpufreq_early_suspend_handler);
#endif
	return cpufreq_register_driver(&msm_cpufreq_driver);
}

device_initcall(msm_cpufreq_register);

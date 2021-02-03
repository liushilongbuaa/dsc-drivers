// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2017 - 2019 Pensando Systems, Inc */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/dynamic_debug.h>
#include <linux/utsname.h>

#include "ionic.h"
#include "ionic_bus.h"
#include "ionic_lif.h"
#include "ionic_debugfs.h"

MODULE_DESCRIPTION(IONIC_DRV_DESCRIPTION);
MODULE_AUTHOR("Pensando Systems, Inc");
MODULE_LICENSE("GPL");
MODULE_VERSION(IONIC_DRV_VERSION);
MODULE_INFO(supported, "external");

unsigned int rx_copybreak = IONIC_RX_COPYBREAK_DEFAULT;
module_param(rx_copybreak, uint, 0600);
MODULE_PARM_DESC(rx_copybreak, "Maximum size of packet that is copied to a bounce buffer on RX");

unsigned int tx_budget = IONIC_TX_BUDGET_DEFAULT;
module_param(tx_budget, uint, 0600);
MODULE_PARM_DESC(tx_budget, "Number of tx completions to process per NAPI poll");

unsigned int devcmd_timeout = DEVCMD_TIMEOUT;
module_param(devcmd_timeout, uint, 0600);
MODULE_PARM_DESC(devcmd_timeout, "Devcmd timeout in seconds (default 30 secs)");

unsigned long affinity_mask_override;
module_param(affinity_mask_override, ulong, 0600);
MODULE_PARM_DESC(affinity_mask_override, "IRQ affinity mask to override (max 64 bits)");

static const char *ionic_error_to_str(enum ionic_status_code code)
{
	switch (code) {
	case IONIC_RC_SUCCESS:
		return "IONIC_RC_SUCCESS";
	case IONIC_RC_EVERSION:
		return "IONIC_RC_EVERSION";
	case IONIC_RC_EOPCODE:
		return "IONIC_RC_EOPCODE";
	case IONIC_RC_EIO:
		return "IONIC_RC_EIO";
	case IONIC_RC_EPERM:
		return "IONIC_RC_EPERM";
	case IONIC_RC_EQID:
		return "IONIC_RC_EQID";
	case IONIC_RC_EQTYPE:
		return "IONIC_RC_EQTYPE";
	case IONIC_RC_ENOENT:
		return "IONIC_RC_ENOENT";
	case IONIC_RC_EINTR:
		return "IONIC_RC_EINTR";
	case IONIC_RC_EAGAIN:
		return "IONIC_RC_EAGAIN";
	case IONIC_RC_ENOMEM:
		return "IONIC_RC_ENOMEM";
	case IONIC_RC_EFAULT:
		return "IONIC_RC_EFAULT";
	case IONIC_RC_EBUSY:
		return "IONIC_RC_EBUSY";
	case IONIC_RC_EEXIST:
		return "IONIC_RC_EEXIST";
	case IONIC_RC_EINVAL:
		return "IONIC_RC_EINVAL";
	case IONIC_RC_ENOSPC:
		return "IONIC_RC_ENOSPC";
	case IONIC_RC_ERANGE:
		return "IONIC_RC_ERANGE";
	case IONIC_RC_BAD_ADDR:
		return "IONIC_RC_BAD_ADDR";
	case IONIC_RC_DEV_CMD:
		return "IONIC_RC_DEV_CMD";
	case IONIC_RC_ENOSUPP:
		return "IONIC_RC_ENOSUPP";
	case IONIC_RC_ERROR:
		return "IONIC_RC_ERROR";
	case IONIC_RC_ERDMA:
		return "IONIC_RC_ERDMA";
	case IONIC_RC_BAD_FW:
		return "IONIC_RC_BAD_FW";
	default:
		return "IONIC_RC_UNKNOWN";
	}
}

int ionic_error_to_errno(enum ionic_status_code code)
{
	switch (code) {
	case IONIC_RC_SUCCESS:
		return 0;
	case IONIC_RC_EVERSION:
	case IONIC_RC_EQTYPE:
	case IONIC_RC_EQID:
	case IONIC_RC_EINVAL:
	case IONIC_RC_ENOSUPP:
		return -EINVAL;
	case IONIC_RC_EPERM:
		return -EPERM;
	case IONIC_RC_ENOENT:
		return -ENOENT;
	case IONIC_RC_EAGAIN:
		return -EAGAIN;
	case IONIC_RC_ENOMEM:
		return -ENOMEM;
	case IONIC_RC_EFAULT:
		return -EFAULT;
	case IONIC_RC_EBUSY:
		return -EBUSY;
	case IONIC_RC_EEXIST:
		return -EEXIST;
	case IONIC_RC_ENOSPC:
		return -ENOSPC;
	case IONIC_RC_ERANGE:
		return -ERANGE;
	case IONIC_RC_BAD_ADDR:
		return -EFAULT;
	case IONIC_RC_BAD_FW:
		return -ENOEXEC;
	case IONIC_RC_EOPCODE:
	case IONIC_RC_EINTR:
	case IONIC_RC_DEV_CMD:
	case IONIC_RC_ERROR:
	case IONIC_RC_ERDMA:
	case IONIC_RC_EIO:
	default:
		return -EIO;
	}
}
EXPORT_SYMBOL_GPL(ionic_error_to_errno);

static const char *ionic_opcode_to_str(enum ionic_cmd_opcode opcode)
{
	switch (opcode) {
	case IONIC_CMD_NOP:
		return "IONIC_CMD_NOP";
	case IONIC_CMD_INIT:
		return "IONIC_CMD_INIT";
	case IONIC_CMD_RESET:
		return "IONIC_CMD_RESET";
	case IONIC_CMD_IDENTIFY:
		return "IONIC_CMD_IDENTIFY";
	case IONIC_CMD_GETATTR:
		return "IONIC_CMD_GETATTR";
	case IONIC_CMD_SETATTR:
		return "IONIC_CMD_SETATTR";
	case IONIC_CMD_PORT_IDENTIFY:
		return "IONIC_CMD_PORT_IDENTIFY";
	case IONIC_CMD_PORT_INIT:
		return "IONIC_CMD_PORT_INIT";
	case IONIC_CMD_PORT_RESET:
		return "IONIC_CMD_PORT_RESET";
	case IONIC_CMD_PORT_GETATTR:
		return "IONIC_CMD_PORT_GETATTR";
	case IONIC_CMD_PORT_SETATTR:
		return "IONIC_CMD_PORT_SETATTR";
	case IONIC_CMD_LIF_INIT:
		return "IONIC_CMD_LIF_INIT";
	case IONIC_CMD_LIF_RESET:
		return "IONIC_CMD_LIF_RESET";
	case IONIC_CMD_LIF_IDENTIFY:
		return "IONIC_CMD_LIF_IDENTIFY";
	case IONIC_CMD_LIF_SETATTR:
		return "IONIC_CMD_LIF_SETATTR";
	case IONIC_CMD_LIF_GETATTR:
		return "IONIC_CMD_LIF_GETATTR";
	case IONIC_CMD_RX_MODE_SET:
		return "IONIC_CMD_RX_MODE_SET";
	case IONIC_CMD_RX_FILTER_ADD:
		return "IONIC_CMD_RX_FILTER_ADD";
	case IONIC_CMD_RX_FILTER_DEL:
		return "IONIC_CMD_RX_FILTER_DEL";
	case IONIC_CMD_Q_IDENTIFY:
		return "IONIC_CMD_Q_IDENTIFY";
	case IONIC_CMD_Q_INIT:
		return "IONIC_CMD_Q_INIT";
	case IONIC_CMD_Q_CONTROL:
		return "IONIC_CMD_Q_CONTROL";
	case IONIC_CMD_RDMA_RESET_LIF:
		return "IONIC_CMD_RDMA_RESET_LIF";
	case IONIC_CMD_RDMA_CREATE_EQ:
		return "IONIC_CMD_RDMA_CREATE_EQ";
	case IONIC_CMD_RDMA_CREATE_CQ:
		return "IONIC_CMD_RDMA_CREATE_CQ";
	case IONIC_CMD_RDMA_CREATE_ADMINQ:
		return "IONIC_CMD_RDMA_CREATE_ADMINQ";
	case IONIC_CMD_FW_DOWNLOAD:
		return "IONIC_CMD_FW_DOWNLOAD";
	case IONIC_CMD_FW_CONTROL:
		return "IONIC_CMD_FW_CONTROL";
	case IONIC_CMD_FW_DOWNLOAD_V1:
		return "IONIC_CMD_FW_DOWNLOAD_V1";
	case IONIC_CMD_FW_CONTROL_V1:
		return "IONIC_CMD_FW_CONTROL_V1";
	case IONIC_CMD_VF_GETATTR:
		return "IONIC_CMD_VF_GETATTR";
	case IONIC_CMD_VF_SETATTR:
		return "IONIC_CMD_VF_SETATTR";
	default:
		return "DEVCMD_UNKNOWN";
	}
}

static void ionic_adminq_flush(struct ionic_lif *lif)
{
	struct ionic_queue *q = &lif->adminqcq->q;
	struct ionic_desc_info *desc_info;

	spin_lock(&lif->adminq_lock);

	while (q->tail_idx != q->head_idx) {
		desc_info = &q->info[q->tail_idx];
		memset(desc_info->desc, 0, sizeof(union ionic_adminq_cmd));
		desc_info->cb = NULL;
		desc_info->cb_arg = NULL;
		q->tail_idx = (q->tail_idx + 1) & (q->num_descs - 1);
	}
	spin_unlock(&lif->adminq_lock);
}

static int ionic_adminq_check_err(struct ionic_lif *lif,
				  struct ionic_admin_ctx *ctx,
				  bool timeout)
{
	struct net_device *netdev = lif->netdev;
	const char *opcode_str;
	const char *status_str;
	int err = 0;

	if (ctx->comp.comp.status || timeout) {
		opcode_str = ionic_opcode_to_str(ctx->cmd.cmd.opcode);
		status_str = ionic_error_to_str(ctx->comp.comp.status);
		err = timeout ? -ETIMEDOUT :
				ionic_error_to_errno(ctx->comp.comp.status);

		netdev_err(netdev, "%s (%d) failed: %s (%d)\n",
			   opcode_str, ctx->cmd.cmd.opcode,
			   timeout ? "TIMEOUT" : status_str, err);

		if (timeout)
			ionic_adminq_flush(lif);
	}

	return err;
}

static void ionic_adminq_cb(struct ionic_queue *q,
			    struct ionic_desc_info *desc_info,
			    struct ionic_cq_info *cq_info, void *cb_arg)
{
	struct ionic_admin_ctx *ctx = cb_arg;
	struct ionic_admin_comp *comp;
	struct device *dev;

	if (!ctx)
		return;

	comp = cq_info->cq_desc;
	dev = q->dev;

	memcpy(&ctx->comp, comp, sizeof(*comp));

	dev_dbg(dev, "comp admin queue command:\n");
	dynamic_hex_dump("comp ", DUMP_PREFIX_OFFSET, 16, 1,
			 &ctx->comp, sizeof(ctx->comp), true);

	complete_all(&ctx->work);
}

int ionic_adminq_post(struct ionic_lif *lif, struct ionic_admin_ctx *ctx)
{
	struct ionic_desc_info *desc_info;
	struct ionic_queue *q;
	int err = 0;

	if (!lif->adminqcq)
		return -EIO;

	q = &lif->adminqcq->q;

	spin_lock(&lif->adminq_lock);
	if (!ionic_q_has_space(q, 1)) {
		err = -ENOSPC;
		goto err_out;
	}

	err = ionic_heartbeat_check(lif->ionic);
	if (err)
		goto err_out;

	desc_info = &q->info[q->head_idx];
	memcpy(desc_info->desc, &ctx->cmd, sizeof(ctx->cmd));

	dev_dbg(&lif->netdev->dev, "post admin queue command:\n");
	dynamic_hex_dump("cmd ", DUMP_PREFIX_OFFSET, 16, 1,
			 &ctx->cmd, sizeof(ctx->cmd), true);

	ionic_q_post(q, true, ionic_adminq_cb, ctx);

err_out:
	spin_unlock(&lif->adminq_lock);

	return err;
}

int ionic_adminq_wait(struct ionic_lif *lif, struct ionic_admin_ctx *ctx, int err)
{
	struct net_device *netdev = lif->netdev;
	unsigned long remaining;
	const char *name;

	if (err) {
		if (!test_bit(IONIC_LIF_F_FW_RESET, lif->state)) {
			name = ionic_opcode_to_str(ctx->cmd.cmd.opcode);
			netdev_err(netdev, "Posting of %s (%d) failed: %d\n",
				   name, ctx->cmd.cmd.opcode, err);
		}
		return err;
	}

	remaining = wait_for_completion_timeout(&ctx->work,
						HZ * (ulong)devcmd_timeout);
	return ionic_adminq_check_err(lif, ctx, (remaining == 0));
}

int ionic_adminq_post_wait(struct ionic_lif *lif, struct ionic_admin_ctx *ctx)
{
	int err;

	err = ionic_adminq_post(lif, ctx);

	return ionic_adminq_wait(lif, ctx, err);
}

static void ionic_dev_cmd_clean(struct ionic *ionic)
{
	union __iomem ionic_dev_cmd_regs *regs = ionic->idev.dev_cmd_regs;

	iowrite32(0, &regs->doorbell);
	memset_io(&regs->cmd, 0, sizeof(regs->cmd));
}

int ionic_dev_cmd_wait(struct ionic *ionic, unsigned long max_seconds)
{
	struct ionic_dev *idev = &ionic->idev;
	unsigned long start_time;
	unsigned long max_wait;
	unsigned long duration;
	int opcode;
	int hb = 0;
	int done;
	int err;

	/* Wait for dev cmd to complete, retrying if we get EAGAIN,
	 * but don't wait any longer than max_seconds.
	 */
	max_wait = jiffies + (max_seconds * HZ);
try_again:
	opcode = ioread8(&idev->dev_cmd_regs->cmd.cmd.opcode);
	start_time = jiffies;
	do {
		done = ionic_dev_cmd_done(idev);
		if (done)
			break;
		usleep_range(100, 200);

		/* Don't check the heartbeat on FW_CONTROL commands as they are
		 * notorious for interrupting the firmware's heartbeat update.
		 */
		if (opcode != IONIC_CMD_FW_CONTROL)
			hb = ionic_heartbeat_check(ionic);
	} while (!done && !hb && time_before(jiffies, max_wait));
	duration = jiffies - start_time;

	dev_dbg(ionic->dev, "DEVCMD %s (%d) done=%d took %ld secs (%ld jiffies)\n",
		ionic_opcode_to_str(opcode), opcode,
		done, duration / HZ, duration);

	if (!done && hb) {
		/* It is possible (but unlikely) that FW was busy and missed a
		 * heartbeat check but is still alive and will process this
		 * request, so don't clean the dev_cmd in this case.
		 */
		dev_warn(ionic->dev, "DEVCMD %s (%d) failed - FW halted\n",
			 ionic_opcode_to_str(opcode), opcode);
		return -ENXIO;
	}

	if (!done && !time_before(jiffies, max_wait)) {
		ionic_dev_cmd_clean(ionic);
		dev_warn(ionic->dev, "DEVCMD %s (%d) timeout after %ld secs\n",
			 ionic_opcode_to_str(opcode), opcode, max_seconds);
		return -ETIMEDOUT;
	}

	err = ionic_dev_cmd_status(&ionic->idev);
	if (err) {
		if (err == IONIC_RC_EAGAIN && !time_after(jiffies, max_wait)) {
			dev_dbg(ionic->dev, "DEV_CMD %s (%d), %s (%d) retrying...\n",
				ionic_opcode_to_str(opcode), opcode,
				ionic_error_to_str(err), err);

			msleep(1000);
			iowrite32(0, &idev->dev_cmd_regs->done);
			iowrite32(1, &idev->dev_cmd_regs->doorbell);
			goto try_again;
		}

		dev_err(ionic->dev, "DEV_CMD %s (%d) error, %s (%d) failed\n",
			ionic_opcode_to_str(opcode), opcode,
			ionic_error_to_str(err), err);

		return ionic_error_to_errno(err);
	}

	return 0;
}

int ionic_set_dma_mask(struct ionic *ionic)
{
	struct device *dev = ionic->dev;
	int err;

	/* Query system for DMA addressing limitation for the device. */
#ifdef CONFIG_PPC64
	ionic->pdev->no_64bit_msi = 1;
#endif
	err = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(IONIC_ADDR_LEN));
	if (err)
		dev_err(dev, "Unable to obtain 64-bit DMA for consistent allocations, aborting.  err=%d\n",
			err);

	return err;
}

int ionic_setup(struct ionic *ionic)
{
	int err;

	err = ionic_dev_setup(ionic);
	if (err)
		return err;

	ionic_debugfs_add_dev_cmd(ionic);
	ionic_reset(ionic);

	return 0;
}

int ionic_identify(struct ionic *ionic)
{
	struct ionic_identity *ident = &ionic->ident;
	struct ionic_dev *idev = &ionic->idev;
	size_t sz;
	int err;

	memset(ident, 0, sizeof(*ident));

	ident->drv.os_type = cpu_to_le32(IONIC_OS_TYPE_LINUX);
	ident->drv.os_dist = 0;
	strncpy(ident->drv.os_dist_str, utsname()->release,
		sizeof(ident->drv.os_dist_str) - 1);
	ident->drv.kernel_ver = cpu_to_le32(LINUX_VERSION_CODE);
	strncpy(ident->drv.kernel_ver_str, utsname()->version,
		sizeof(ident->drv.kernel_ver_str) - 1);
	strncpy(ident->drv.driver_ver_str, IONIC_DRV_VERSION,
		sizeof(ident->drv.driver_ver_str) - 1);

	mutex_lock(&ionic->dev_cmd_lock);

	sz = min(sizeof(ident->drv), sizeof(idev->dev_cmd_regs->data));
	memcpy_toio(&idev->dev_cmd_regs->data, &ident->drv, sz);

	ionic_dev_cmd_identify(idev, IONIC_IDENTITY_VERSION_1);
	err = ionic_dev_cmd_wait(ionic, devcmd_timeout);
	if (!err) {
		sz = min(sizeof(ident->dev), sizeof(idev->dev_cmd_regs->data));
		memcpy_fromio(&ident->dev, &idev->dev_cmd_regs->data, sz);
	}
	mutex_unlock(&ionic->dev_cmd_lock);

	if (err) {
		dev_err(ionic->dev, "Cannot identify ionic: %dn", err);
		goto err_out;
	}

	err = ionic_lif_identify(ionic, IONIC_LIF_TYPE_CLASSIC,
				 &ionic->ident.lif);
	if (err) {
		dev_err(ionic->dev, "Cannot identify LIFs: %d\n", err);
		goto err_out;
	}

	return 0;

err_out:
	return err;
}

int ionic_init(struct ionic *ionic)
{
	struct ionic_dev *idev = &ionic->idev;
	int err;

	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_init(idev);
	err = ionic_dev_cmd_wait(ionic, devcmd_timeout);
	mutex_unlock(&ionic->dev_cmd_lock);

	return err;
}

int ionic_reset(struct ionic *ionic)
{
	struct ionic_dev *idev = &ionic->idev;
	int err;

	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_reset(idev);
	err = ionic_dev_cmd_wait(ionic, devcmd_timeout);
	mutex_unlock(&ionic->dev_cmd_lock);

	return err;
}

int ionic_port_identify(struct ionic *ionic)
{
	struct ionic_identity *ident = &ionic->ident;
	struct ionic_dev *idev = &ionic->idev;
	struct device *dev = ionic->dev;
	size_t sz;
	int err;

	mutex_lock(&ionic->dev_cmd_lock);

	ionic_dev_cmd_port_identify(idev);
	err = ionic_dev_cmd_wait(ionic, devcmd_timeout);
	if (!err) {
		sz = min(sizeof(ident->port), sizeof(idev->dev_cmd_regs->data));
		memcpy_fromio(&ident->port, &idev->dev_cmd_regs->data, sz);
	}

	mutex_unlock(&ionic->dev_cmd_lock);

	dev_dbg(dev, "type %d\n", ident->port.type);
	dev_dbg(dev, "speed %d\n", ident->port.config.speed);
	dev_dbg(dev, "mtu %d\n", ident->port.config.mtu);
	dev_dbg(dev, "state %d\n", ident->port.config.state);
	dev_dbg(dev, "an_enable %d\n", ident->port.config.an_enable);
	dev_dbg(dev, "fec_type %d\n", ident->port.config.fec_type);
	dev_dbg(dev, "pause_type %d\n", ident->port.config.pause_type);
	dev_dbg(dev, "loopback_mode %d\n", ident->port.config.loopback_mode);

	return err;
}

int ionic_port_init(struct ionic *ionic)
{
	struct ionic_identity *ident = &ionic->ident;
	struct ionic_dev *idev = &ionic->idev;
	size_t sz;
	int err;

	if (!idev->port_info) {
		idev->port_info_sz = ALIGN(sizeof(*idev->port_info), PAGE_SIZE);
		idev->port_info = dma_alloc_coherent(ionic->dev,
						     idev->port_info_sz,
						     &idev->port_info_pa,
						     GFP_KERNEL);
		if (!idev->port_info) {
			dev_err(ionic->dev, "Failed to allocate port info\n");
			return -ENOMEM;
		}
	}

	sz = min(sizeof(ident->port.config), sizeof(idev->dev_cmd_regs->data));

	mutex_lock(&ionic->dev_cmd_lock);

	memcpy_toio(&idev->dev_cmd_regs->data, &ident->port.config, sz);
	ionic_dev_cmd_port_init(idev);
	err = ionic_dev_cmd_wait(ionic, devcmd_timeout);

	ionic_dev_cmd_port_state(&ionic->idev, IONIC_PORT_ADMIN_STATE_UP);
	(void)ionic_dev_cmd_wait(ionic, devcmd_timeout);

	mutex_unlock(&ionic->dev_cmd_lock);
	if (err) {
		dev_err(ionic->dev, "Failed to init port\n");
		dma_free_coherent(ionic->dev, idev->port_info_sz,
				  idev->port_info, idev->port_info_pa);
		idev->port_info = NULL;
		idev->port_info_pa = 0;
	}

	return err;
}

int ionic_port_reset(struct ionic *ionic)
{
	struct ionic_dev *idev = &ionic->idev;
	int err;

	if (!idev->port_info)
		return 0;

	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_port_reset(idev);
	err = ionic_dev_cmd_wait(ionic, devcmd_timeout);
	mutex_unlock(&ionic->dev_cmd_lock);

	dma_free_coherent(ionic->dev, idev->port_info_sz,
			  idev->port_info, idev->port_info_pa);

	idev->port_info = NULL;
	idev->port_info_pa = 0;

	if (err)
		dev_err(ionic->dev, "Failed to reset port\n");

	return err;
}

static int __init ionic_init_module(void)
{
	unsigned long max_affinity = GENMASK_ULL((min(num_present_cpus(),
					(unsigned int)(sizeof(unsigned long)*BITS_PER_BYTE))-1), 0);

	pr_info("%s %s, ver %s\n",
		IONIC_DRV_NAME, IONIC_DRV_DESCRIPTION, IONIC_DRV_VERSION);

	ionic_debugfs_create();

	if (affinity_mask_override) {
		/* limit affinity mask override to the available CPUs */
		if (affinity_mask_override > max_affinity) {
			affinity_mask_override = (affinity_mask_override & max_affinity);
			pr_info("limiting affinity mask to: 0x%lx\n",
					affinity_mask_override);
		} else {
			pr_info("affinity_mask_override: %lx\n",
					affinity_mask_override);
		}
	}

	return ionic_bus_register_driver();
}

static void __exit ionic_cleanup_module(void)
{
	/* If there's a long devcmd_timeout set, don't let
	 * hung FW slow us down when exiting
	 */
	devcmd_timeout = min_t(int, devcmd_timeout, SHORT_TIMEOUT);

	ionic_bus_unregister_driver();
	ionic_debugfs_destroy();

	pr_info("%s removed\n", IONIC_DRV_NAME);
}

module_init(ionic_init_module);
module_exit(ionic_cleanup_module);

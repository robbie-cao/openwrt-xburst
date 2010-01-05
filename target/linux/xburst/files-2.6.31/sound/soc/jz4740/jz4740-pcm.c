/*
 *  Copyright (C) 2010, Lars-Peter Clausen <lars@metafoo.de>
 *
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/mach-jz4740/dma.h>
#include <asm/mach-jz4740/regs.h>
#include "jz4740-pcm.h"

struct jz4740_runtime_data {
	unsigned int dma_period;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	struct jz4740_dma_chan *dma;
};

static struct jz4740_dma_config jz4740_pcm_dma_playback_config = {
	.src_width = JZ4740_DMA_WIDTH_16BIT,
	.dst_width = JZ4740_DMA_WIDTH_32BIT,
	.transfer_size = JZ4740_DMA_TRANSFER_SIZE_16BYTE,
	.request_type = JZ4740_DMA_TYPE_AIC_TRANSMIT,
	.flags = JZ4740_DMA_SRC_AUTOINC,
	.mode = JZ4740_DMA_MODE_SINGLE,
};

static struct jz4740_dma_config jz4740_pcm_dma_capture_config = {
	.src_width = JZ4740_DMA_WIDTH_32BIT,
	.dst_width = JZ4740_DMA_WIDTH_16BIT,
	.transfer_size = JZ4740_DMA_TRANSFER_SIZE_16BYTE,
	.request_type = JZ4740_DMA_TYPE_AIC_RECEIVE,
	.flags = JZ4740_DMA_DST_AUTOINC,
	.mode = JZ4740_DMA_MODE_SINGLE,
};

/* identify hardware playback capabilities */
static const struct snd_pcm_hardware jz4740_pcm_hardware = {
	.info = SNDRV_PCM_INFO_MMAP |
	        SNDRV_PCM_INFO_MMAP_VALID |
	        SNDRV_PCM_INFO_INTERLEAVED |
	        SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.formats = SNDRV_PCM_FMTBIT_S16_LE |
	           SNDRV_PCM_FMTBIT_S8,
	.rates                  = SNDRV_PCM_RATE_8000_48000,
	.channels_min		= 1,
	.channels_max		= 2,
	.period_bytes_min	= 32,
	.period_bytes_max	= 2 * PAGE_SIZE,
	.periods_min		= 2,
	.periods_max		= 128,
	.buffer_bytes_max	= 128 * 2 * PAGE_SIZE,
	.fifo_size			= 32,
};

static void jz4740_pcm_start_transfer(struct jz4740_runtime_data *prtd, int stream)
{
	unsigned int count;

	if (prtd->dma_pos + prtd->dma_period > prtd->dma_end)
		count = prtd->dma_end - prtd->dma_pos;
	else
		count = prtd->dma_period;

	jz4740_dma_disable(prtd->dma);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		jz4740_dma_set_src_addr(prtd->dma, prtd->dma_pos);
		jz4740_dma_set_dst_addr(prtd->dma, CPHYSADDR(AIC_DR));
	} else {
		jz4740_dma_set_src_addr(prtd->dma, CPHYSADDR(AIC_DR));
		jz4740_dma_set_dst_addr(prtd->dma, prtd->dma_pos);
	}

	jz4740_dma_set_transfer_count(prtd->dma, count);

	jz4740_dma_enable(prtd->dma);

	prtd->dma_pos += prtd->dma_period;
	if (prtd->dma_pos >= prtd->dma_end)
		prtd->dma_pos = prtd->dma_start;
}

static void jz4740_pcm_dma_transfer_done(struct jz4740_dma_chan *dma, int err,
                                         void *dev_id)
{
	struct snd_pcm_substream *substream = dev_id;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct jz4740_runtime_data *prtd = runtime->private_data;

	snd_pcm_period_elapsed(substream);

	jz4740_pcm_start_transfer(prtd, substream->stream);
}

static int jz4740_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct jz4740_runtime_data *prtd = runtime->private_data;
	unsigned int size = params_buffer_bytes(params);
	struct jz4740_dma_config *dma_config;
	enum jz4740_dma_width width;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		width = JZ4740_DMA_WIDTH_8BIT;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		width = JZ4740_DMA_WIDTH_16BIT;
		break;
	default:
		BUG();
		break;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_config = &jz4740_pcm_dma_playback_config;
		dma_config->src_width = width;

		prtd->dma = jz4740_dma_request(substream, "PCM Playback");
	} else {
		dma_config = &jz4740_pcm_dma_capture_config;
		dma_config->dst_width = width;

		prtd->dma = jz4740_dma_request(substream, "PCM Capture");
	}

	if (!prtd->dma)
		return -EBUSY;

	jz4740_dma_configure(prtd->dma, dma_config);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		jz4740_dma_set_dst_addr(prtd->dma, CPHYSADDR(AIC_DR));
	else
		jz4740_dma_set_src_addr(prtd->dma, CPHYSADDR(AIC_DR));

	jz4740_dma_set_complete_cb(prtd->dma, jz4740_pcm_dma_transfer_done);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = size;

	prtd->dma_period = params_period_bytes(params);
	prtd->dma_start = runtime->dma_addr;
	prtd->dma_pos = prtd->dma_start;
	prtd->dma_end = prtd->dma_start + size;

	return 0;
}

static int jz4740_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct jz4740_runtime_data *prtd = substream->runtime->private_data;

	snd_pcm_set_runtime_buffer(substream, NULL);
	if (prtd->dma)
		jz4740_dma_free(prtd->dma);

	return 0;
}

static int jz4740_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct jz4740_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;

	if (!prtd->dma)
	 	return 0;

	prtd->dma_pos = prtd->dma_start;

	return ret;
}

static int jz4740_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct jz4740_runtime_data *prtd = runtime->private_data;

	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		jz4740_pcm_start_transfer(prtd, substream->stream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		jz4740_dma_disable(prtd->dma);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static snd_pcm_uframes_t jz4740_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct jz4740_runtime_data *prtd = runtime->private_data;
	unsigned long count, pos;
	snd_pcm_uframes_t offset;
	struct jz4740_dma_chan *dma = prtd->dma;

	count = jz4740_dma_get_residue(dma);
	if (prtd->dma_pos == prtd->dma_start)
		pos = prtd->dma_end - prtd->dma_start - count;
	else
		pos = prtd->dma_pos - prtd->dma_start - count;

	offset = bytes_to_frames(runtime, pos);
	if (offset >= runtime->buffer_size)
		offset = 0;

	return offset;
}

static int jz4740_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct jz4740_runtime_data *prtd;

	snd_soc_set_runtime_hwparams(substream, &jz4740_pcm_hardware);
	prtd = kzalloc(sizeof(struct jz4740_runtime_data), GFP_KERNEL);

	if (prtd == NULL)
		return -ENOMEM;

	runtime->private_data = prtd;
	return 0;
}

static int jz4740_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct jz4740_runtime_data *prtd = runtime->private_data;

	kfree(prtd);

	return 0;
}

static int jz4740_pcm_mmap(struct snd_pcm_substream *substream,
			   struct vm_area_struct *vma)
{
	return remap_pfn_range(vma, vma->vm_start,
		       substream->dma_buffer.addr >> PAGE_SHIFT,
		       vma->vm_end - vma->vm_start, vma->vm_page_prot);
}

struct snd_pcm_ops jz4740_pcm_ops = {
	.open		= jz4740_pcm_open,
	.close		= jz4740_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= jz4740_pcm_hw_params,
	.hw_free	= jz4740_pcm_hw_free,
	.prepare	= jz4740_pcm_prepare,
	.trigger	= jz4740_pcm_trigger,
	.pointer	= jz4740_pcm_pointer,
	.mmap		= jz4740_pcm_mmap,
};

static int jz4740_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = jz4740_pcm_hardware.buffer_bytes_max;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;

	buf->area = dma_alloc_noncoherent(pcm->card->dev, size,
					  &buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;

	buf->bytes = size;

	return 0;
}

static void jz4740_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_noncoherent(pcm->card->dev, buf->bytes,
		  buf->area, buf->addr);
		buf->area = NULL;
	}
}

static u64 jz4740_pcm_dmamask = DMA_BIT_MASK(32);

int jz4740_pcm_new(struct snd_card *card, struct snd_soc_dai *dai,
	struct snd_pcm *pcm)
{
	int ret = 0;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &jz4740_pcm_dmamask;

	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (dai->playback.channels_min) {
		ret = jz4740_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto err;
	}

	if (dai->capture.channels_min) {
		ret = jz4740_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto err;
	}

err:
	return ret;
}

struct snd_soc_platform jz4740_soc_platform = {
	.name		= "jz4740-pcm",
	.pcm_ops 	= &jz4740_pcm_ops,
	.pcm_new	= jz4740_pcm_new,
	.pcm_free	= jz4740_pcm_free_dma_buffers,
};
EXPORT_SYMBOL_GPL(jz4740_soc_platform);

static int __init jz4740_soc_platform_init(void)
{
    return snd_soc_register_platform(&jz4740_soc_platform);
}
module_init(jz4740_soc_platform_init);

static void __exit jz4740_soc_platform_exit(void)
{
    snd_soc_unregister_platform(&jz4740_soc_platform);
}
module_exit(jz4740_soc_platform_exit);

MODULE_AUTHOR("Lars-Peter Clausen <lars@metafoo.de>");
MODULE_DESCRIPTION("Ingenic SoC JZ4740 PCM driver");
MODULE_LICENSE("GPL");

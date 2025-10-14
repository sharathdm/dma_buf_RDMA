/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include <linux/dma-buf.h>
#include <linux/highmem.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>

#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include "ion.h"

static struct device *ion_dev;
int errorType;

struct ion_data {
        int npages;
        struct page *pages[];
};

static int ion_attach(struct dma_buf *dmabuf,
                        struct dma_buf_attachment *attachment)
{
        printk("dmabuf attach device: \n");
        return 0;
}

static void ion_detach(struct dma_buf *dmabuf, struct dma_buf_attachment *attachment)
{
        printk("dmabuf detach device: %s\n", dev_name(attachment->dev));
}

static int ion_pin(struct dma_buf_attachment *attachment) {
        printk("inside ion_pin\n");
        return 0;
}

static void ion_unpin(struct dma_buf_attachment *attachment) {
        printk("inside ion_unpin\n");
}

static struct sg_table *ion_map_dma_buf(struct dma_buf_attachment *attachment,
                                         enum dma_data_direction dir)
{
        struct ion_data *data = attachment->dmabuf->priv;
        struct sg_table *table;
        struct scatterlist *sg;
        int i;

        printk("ion_map_dma_buf\n");

        table = kmalloc(sizeof(*table), GFP_KERNEL);
        if (!table)
                return ERR_PTR(-ENOMEM);

        if (sg_alloc_table(table, data->npages, GFP_KERNEL)) {
                kfree(table);
                return ERR_PTR(-ENOMEM);
        }

        sg = table->sgl;
        for (i = 0; i < data->npages; i++) {
                sg_set_page(sg, data->pages[i], PAGE_SIZE, 0);
                sg = sg_next(sg);
        }

        if (!dma_map_sg(ion_dev, table->sgl, table->nents, dir)) {
                sg_free_table(table);
                kfree(table);
                return ERR_PTR(-ENOMEM);
        }

       sg = table->sgl;
        do {
        //        printk("offset %x length %x dma addr %pad\n", sg->offset, sg->length, &sg->dma_address);
                sg =sg_next(sg);
        } while(sg);


        return table;
}

static void ion_unmap_dma_buf(struct dma_buf_attachment *attachment,
                               struct sg_table *table,
                               enum dma_data_direction dir)
{
        printk("ion_unmap_dma_buf\n");
        dma_unmap_sg(ion_dev, table->sgl, table->nents, dir);
        sg_free_table(table);
        kfree(table);
}

static void ion_release(struct dma_buf *dma_buf)
{
        struct ion_data *data = dma_buf->priv;
        int i;

        printk("dmabuf release\n");

        for (i = 0; i < data->npages; i++) {
                if(data->pages[i]) {
//                        put_page(data->pages[i]);
                        __free_page(data->pages[i]);
                }
        }
        kfree(data);
}

static int ion_mmap(struct dma_buf *dma_buf, struct vm_area_struct *vma)
{
        struct ion_data *data = dma_buf->priv;
        unsigned long vm_start = vma->vm_start;
        int i;

        for (i = 0; i < data->npages; i++) {
                remap_pfn_range(vma, vm_start, page_to_pfn(data->pages[i]),
                                PAGE_SIZE, vma->vm_page_prot);
                vm_start += PAGE_SIZE;
        }

        return 0;
}

static int ion_begin_cpu_access(struct dma_buf *dmabuf,
                                      enum dma_data_direction dir)
{
        struct dma_buf_attachment *attachment;
        struct sg_table *table;

        if (list_empty(&dmabuf->attachments))
                return 0;

        attachment = list_first_entry(&dmabuf->attachments, struct dma_buf_attachment, node);
        table = attachment->priv;
        dma_sync_sg_for_cpu(NULL, table->sgl, table->nents, dir);

        return 0;
}

static int ion_end_cpu_access(struct dma_buf *dmabuf,
                                enum dma_data_direction dir)
{
        struct dma_buf_attachment *attachment;
        struct sg_table *table;

        if (list_empty(&dmabuf->attachments))
                return 0;

        attachment = list_first_entry(&dmabuf->attachments, struct dma_buf_attachment, node);
        table = attachment->priv;
        dma_sync_sg_for_device(NULL, table->sgl, table->nents, dir);

        return 0;
}

static const struct dma_buf_ops exp_dmabuf_ops = {
                .attach = ion_attach,
        .detach = ion_detach,
        .map_dma_buf = ion_map_dma_buf,
        .unmap_dma_buf = ion_unmap_dma_buf,
        .release = ion_release,
        .mmap = ion_mmap,
        .begin_cpu_access = ion_begin_cpu_access,
        .end_cpu_access = ion_end_cpu_access,
        .pin = ion_pin,
        .unpin = ion_unpin,
};

static struct dma_buf *ion_alloc(size_t size)
{
        DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
        struct dma_buf *dmabuf;
        struct ion_data *data;
        int i, npages;

        npages = PAGE_ALIGN(size) / PAGE_SIZE;
        if (!npages)
                return ERR_PTR(-EINVAL);

        printk("alloc_page\n");
        data = kmalloc(sizeof(*data) + npages * sizeof(struct page *),
                       GFP_KERNEL);
        if (!data)
                return ERR_PTR(-ENOMEM);

        for (i = 0; i < npages; i++) {
                data->pages[i] = alloc_page(GFP_KERNEL);
                if (!data->pages[i])
                        goto err;
        }
        data->npages = npages;

        exp_info.ops = &exp_dmabuf_ops;
        exp_info.size = npages * PAGE_SIZE;
        exp_info.flags = O_RDWR;
        exp_info.priv = data;

        dmabuf = dma_buf_export(&exp_info);
        if (IS_ERR(dmabuf))
                goto err;

        return dmabuf;

err:
        printk("memory allocation failed\n");

//        while (i--)
 //               put_page(data->pages[i]);
        kfree(data);
        return ERR_PTR(-ENOMEM);
}

static long ion_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
        struct dma_buf *dmabuf;
        struct ion_allocation_data alloc_data;

        if (copy_from_user(&alloc_data, (void __user *)arg, sizeof(alloc_data)))
                return -EFAULT;

        if (cmd != ION_IOC_ALLOC) {
                pr_err("ioctl %u is not supported!", cmd);
                return -EINVAL;
        }

        dmabuf = ion_alloc(alloc_data.len);
        if (!dmabuf) {
                pr_err("error: exporter alloc page failed\n");
                return -ENOMEM;
        }

        alloc_data.fd = dma_buf_fd(dmabuf, O_RDWR);

        if (copy_to_user((void __user *)arg, &alloc_data, sizeof(alloc_data)))
                return -EFAULT;

        return 0;
}

static dev_t ion_dev_number;
static struct cdev driver_object;
static struct class *ion_class;

static struct file_operations ion_fops = {
    .owner= THIS_MODULE,
        .unlocked_ioctl   = ion_ioctl,
};

u64 dma_mask=0xffffffff;
static int __init ion_init(void)
{
        int debug;
    if (alloc_chrdev_region(&ion_dev_number,0,1,"ion")<0)
        return -EIO;

    cdev_init(&driver_object, &ion_fops);

    if (cdev_add(&driver_object,ion_dev_number,1)){
        errorType=EIO;
        goto free_cdev;
    }

    ion_class = class_create("ion_class");
    if (IS_ERR( ion_class )) {
        pr_err( "ion: no udev support\n");
        errorType=EIO;
        goto free_cdev;
    }
    ion_dev = device_create( ion_class, NULL, ion_dev_number, NULL, "%s", "ion" );
    if (IS_ERR( ion_dev )) {
        pr_err( "ion: device_create failed\n");
        errorType=EIO;
        goto free_class;
    }

    ion_dev->dma_mask = &dma_mask;
    debug = dma_set_mask_and_coherent(ion_dev, DMA_BIT_MASK(32));

    return 0;
free_class:
free_cdev:
free_device_number:
        return -1;

}

static void __exit ion_exit(void)
{
            device_destroy( ion_class, ion_dev_number );
    class_destroy( ion_class );
    cdev_del( &driver_object );
    unregister_chrdev_region( ion_dev_number, 1 );

}

module_init(ion_init);
module_exit(ion_exit);

MODULE_AUTHOR("Leon He <343005384@qq.com>");
MODULE_DESCRIPTION("DMA-BUF exporter example for my ion");
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(DMA_BUF);
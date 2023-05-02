#include <kernel/vmem.h>
#include <kernel/kmem.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void vmem_heap_expand(vmem_heap_t *heap, size_t new_sz);
static int vmem_header_compar(ord_arr_type_t a, ord_arr_type_t b);
static size_t vmem_heap_contract(vmem_heap_t *heap, size_t new_sz);
static size_t vmem_heap_hole_find(vmem_heap_t *heap, size_t sz, uint8_t align);

vmem_heap_t kheap = {0};

/* heap */
static int vmem_header_compar(ord_arr_type_t a, ord_arr_type_t b) {
    return ((vmem_header_t *)a)->size < ((vmem_header_t *)b)->size;
}

void vmem_heap_create(vmem_heap_t *heap, void *start, void *end, void *max, size_t index_sz, uint8_t kernel) {
    /* vmem_heap_t *heap = (vmem_heap_t *)kmalloc(sizeof(vmem_heap_t)); */

    assert((uintptr_t)start % PAGE_SIZE == 0);
    assert((uintptr_t)end % PAGE_SIZE == 0);

    heap->kernel = kernel;

    ord_arr_place(&heap->index, start, index_sz, &vmem_header_compar);
    start += sizeof(ord_arr_type_t) * index_sz;

    /* align start */
    start = (void *)((uintptr_t)(start + PAGE_SIZE-1) & ~(PAGE_SIZE-1));

    heap->start = start;
    heap->end = end;
    heap->max = max;

    /* create one large hole */
    vmem_header_t *hole = (vmem_header_t *)start;
    hole->size = end - start;
    hole->magic = VMEM_HEAP_MAGIC;
    hole->align = 0;
    hole->hole = 1;

    vmem_footer_t *ftr = end - sizeof(vmem_footer_t);
    ftr->magic = VMEM_HEAP_MAGIC;
    ftr->hdr = hole;

    ord_arr_insert(&heap->index, hole);
}

static void vmem_heap_expand(vmem_heap_t *heap, size_t new_sz) {
    /* align and check new_sz */
    new_sz = (new_sz + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
    assert(new_sz > (size_t)(heap->end - heap->start));
    assert(heap->start + new_sz <= heap->max);

    unsigned get_flags = VMEM_GET_CREATE, alloc_flags = VMEM_FLAG_WRITE;
    if(heap->kernel) get_flags |= VMEM_GET_KERNEL, alloc_flags |= VMEM_FLAG_KERNEL;

    for(size_t old_sz = heap->end - heap->start; old_sz < new_sz; old_sz += PAGE_SIZE)
        vmem_frame_alloc(vmem_get_page((uintptr_t)heap->start+old_sz,
                                       get_flags), alloc_flags);
    heap->end = heap->start + new_sz;
}

/* unused as of now */
static size_t __unused vmem_heap_contract(vmem_heap_t *heap, size_t new_sz) {
    new_sz = (new_sz + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
    if(new_sz < VMEM_HEAP_INITIAL_SZ) new_sz = VMEM_HEAP_INITIAL_SZ;
    assert(new_sz < (size_t)(heap->end - heap->start));

    size_t old_sz = heap->end - heap->start;
    while(old_sz > new_sz) {
        vmem_frame_free(vmem_get_page((uintptr_t)heap->start+old_sz,
                                      VMEM_GET_CREATE|VMEM_GET_KERNEL));
        old_sz -= PAGE_SIZE;
    }
    heap->end = heap->start + new_sz;
    return new_sz;
}

static size_t vmem_heap_hole_find(vmem_heap_t *heap, size_t size, uint8_t align) {
    for(size_t iter = 0; iter < heap->index.size; iter++) {
        vmem_header_t *header;
        size_t hole_sz;

        header = (vmem_header_t *)ord_arr_get(&heap->index, iter);
        hole_sz = header->size;

        if(align) {
            uintptr_t loc;
            size_t off;

            /* align location AFTER the header */
            loc = (uintptr_t)header + sizeof(vmem_header_t);
            off = 0;
            if(loc & ((1<<align)-1))
                off = (1<<align) - (loc & ((1<<align)-1));
            if(off < sizeof(vmem_header_t)) off += 1<<align;
            if(off > hole_sz) continue;
            hole_sz -= off;
        }
        if(hole_sz >= size)
            return iter;
    }
    /* not found */
    return SIZE_MAX;
}

void *vmem_heap_alloc(vmem_heap_t *heap, size_t size, uint8_t align) {
    size_t real_size, hole_size, hole;
    vmem_header_t *orig_header;

    real_size = size + sizeof(vmem_header_t) + sizeof(vmem_footer_t);
    hole = vmem_heap_hole_find(heap, real_size, align);

    if(hole == SIZE_MAX) {
        size_t heap_newsz, heap_oldsz = heap->end - heap->start;
        void *heap_oldend = heap->end;

        vmem_heap_expand(heap, heap_oldsz + real_size);
        heap_newsz = heap->end - heap->start;

        size_t idx = SIZE_MAX;
        void *cur_max = NULL;
        for(size_t i = 0; i < heap->index.size; i++) {
            void *tmp = ord_arr_get(&heap->index, i);
            if(tmp > cur_max) {
                cur_max = tmp;
                idx = i;
            }
        }

        if(idx == SIZE_MAX) {
            vmem_header_t *hdr = (vmem_header_t *)heap_oldend;
            hdr->magic = VMEM_HEAP_MAGIC;
            hdr->size = heap_newsz - heap_oldsz;
            hdr->hole = 1;
            hdr->align = 0;

            vmem_footer_t *ftr = (vmem_footer_t *)((void *)hdr + hdr->size - sizeof(vmem_footer_t));
            ftr->magic = VMEM_HEAP_MAGIC;
            ftr->hdr = hdr;

            ord_arr_insert(&heap->index, hdr);
        } else {
            vmem_header_t *hdr = cur_max;
            hdr->magic = VMEM_HEAP_MAGIC;
            hdr->size += heap_newsz - heap_oldsz;

            vmem_footer_t *ftr = (vmem_footer_t *)((void *)hdr + hdr->size - sizeof(vmem_footer_t));
            ftr->magic = VMEM_HEAP_MAGIC;
            ftr->hdr = hdr;
        }

        return vmem_heap_alloc(heap, size, align);
    }

    orig_header = ord_arr_get(&heap->index, hole);
    hole_size = orig_header->size;
    if(hole_size - real_size < sizeof(vmem_header_t) + sizeof(vmem_footer_t)) {
        real_size = hole_size;
    }

    /* align and create a hole before */
    if(align) {
        uintptr_t off = (1<<align) - ((uintptr_t)orig_header & ((1<<align)-1));
        /* make sure the header fits */
        if(off < sizeof(vmem_header_t))
            off += 1<<align;

        void *new_loc = (void *)orig_header + off - sizeof(vmem_header_t);

        orig_header->size = off - sizeof(vmem_header_t);
        orig_header->magic = VMEM_HEAP_MAGIC;
        orig_header->hole = 1;
        orig_header->align = 0;

        vmem_footer_t *footer = new_loc - sizeof(vmem_footer_t);
        footer->magic = VMEM_HEAP_MAGIC;
        footer->hdr = orig_header;

        hole_size -= orig_header->size;
        orig_header = new_loc;
    } else {
        ord_arr_remove(&heap->index, hole);
    }

    orig_header->size = real_size;
    orig_header->magic = VMEM_HEAP_MAGIC;
    orig_header->hole = 0;
    orig_header->align = align;
    vmem_footer_t *footer = (void *)orig_header + real_size - sizeof(vmem_footer_t);
    footer->magic = VMEM_HEAP_MAGIC;
    footer->hdr = orig_header;

    if(hole_size - real_size > 0) {
        vmem_header_t *header = (void *)orig_header + orig_header->size;
        header->size = hole_size - real_size;
        header->magic = VMEM_HEAP_MAGIC;
        header->hole = 1;
        header->align = 0;

        vmem_footer_t *footer = (void *)header + header->size - sizeof(vmem_footer_t);
        if((uintptr_t)footer < (uintptr_t)heap->end) {
            footer->magic = VMEM_HEAP_MAGIC;
            footer->hdr = header;
        }

        ord_arr_insert(&heap->index, header);
    }

    return (void *)orig_header + sizeof(vmem_header_t);
}

void *vmem_heap_realloc(vmem_heap_t *heap, void *old, size_t size) {
    if(!old) return vmem_heap_alloc(heap, size, 0);
    if(!size) { vmem_heap_free(heap, old); return NULL; }

    /* this is more useful in calculations */
    size += sizeof(vmem_header_t) + sizeof(vmem_footer_t);

    vmem_header_t *hdr = old - sizeof(vmem_header_t);
    vmem_header_t *ftr = (void *)hdr + hdr->size - sizeof(vmem_footer_t);
    vmem_header_t *rhdr = (void *)ftr + sizeof(vmem_footer_t);

    assert(hdr->magic == VMEM_HEAP_MAGIC);
    assert(ftr->magic == VMEM_HEAP_MAGIC);
    assert(hdr->hole == 0 && "realloc on freed segment");

    if(hdr->size == size) return old;

    /* shrink, new < old */
    if(hdr->size > size) {
        if((void *)rhdr + sizeof(vmem_header_t) <= heap->end && rhdr->hole) {
            /* extend right hole */
            size_t i = ord_arr_index(&heap->index, rhdr);
            assert(i != SIZE_MAX && "Hole not in index");
            ord_arr_remove(&heap->index, i);

            vmem_header_t *new_rhdr = (void *)hdr + size;
            new_rhdr->magic = VMEM_HEAP_MAGIC;
            new_rhdr->align = 0;
            new_rhdr->size = rhdr->size + (size - hdr->size);
            new_rhdr->hole = 1;

            vmem_footer_t *old_rftr = (void *)rhdr + rhdr->size - sizeof(vmem_footer_t);
            old_rftr->hdr = new_rhdr;

            ord_arr_insert(&heap->index, new_rhdr);
        } else if(size - hdr->size >= sizeof(vmem_header_t) + sizeof(vmem_footer_t)) {
            /* shrink if a new hole will fit */
            vmem_header_t *hole = (void *)hdr + size;
            hole->magic = VMEM_HEAP_MAGIC;
            hole->hole = 1;
            hole->size = size - hdr->size;
            hole->align = 0;

            vmem_footer_t *hole_ftr = (void *)hole + hole->size - sizeof(vmem_footer_t);
            hole_ftr->magic = VMEM_HEAP_MAGIC;
            hole_ftr->hdr = hole;

            ord_arr_insert(&heap->index, hole);
        } else {
            /* if a new hole won't fit, keep the old size */
            return old;
        }

        /* add new footer */
        hdr->size = size;
        vmem_footer_t *new_ftr = (void *)hdr + hdr->size - sizeof(vmem_footer_t);
        new_ftr->magic = VMEM_HEAP_MAGIC;
        new_ftr->hdr = hdr;
        return old;
    }

    /* extend, new > old */
    if((void *)rhdr + sizeof(vmem_header_t) > heap->end) {
        /* expand heap */
        size_t heap_oldsz = heap->end - heap->start;
        vmem_heap_expand(heap, heap_oldsz + size);

        hdr->size = size;
        vmem_footer_t *new_ftr = (void *)hdr + hdr->size - sizeof(vmem_footer_t);
        new_ftr->magic = VMEM_HEAP_MAGIC;
        new_ftr->hdr = hdr;

        /* after expansion we are guaranteed to fit a new hole */
        vmem_header_t *hole = (void *)new_ftr + sizeof(vmem_footer_t);
        hole->magic = VMEM_HEAP_MAGIC;
        hole->hole = 1;
        hole->size = (uintptr_t)heap->end - (uintptr_t)hole;
        hole->align = 0;

        vmem_footer_t *hole_ftr = (void *)hole + hole->size - sizeof(vmem_footer_t);
        hole_ftr->magic = VMEM_HEAP_MAGIC;
        hole_ftr->hdr = hole;

        ord_arr_insert(&heap->index, hole);

        return old;
    }

    if(rhdr->hole) {
        /* merge with right hole */
        vmem_footer_t *rftr = (void *)rhdr + rhdr->size - sizeof(vmem_footer_t);

        size_t extra_size = size - hdr->size;
        size_t i = ord_arr_index(&heap->index, rhdr);
        ord_arr_remove(&heap->index, i);
        if(rhdr->size < extra_size) {
            /* if new allocation is larger than the hole then
             * claim the hole and recurse */
            hdr->size += rhdr->size;
            rftr->hdr = hdr;

            return vmem_heap_realloc(heap, old, size);
        } else if(rhdr->size - extra_size >= sizeof(vmem_header_t) + sizeof(vmem_footer_t)) {
            /* a new hole fits */
            hdr->size = size;
            vmem_footer_t *new_ftr = (void *)hdr + hdr->size - sizeof(vmem_footer_t);
            new_ftr->magic = VMEM_HEAP_MAGIC;
            new_ftr->hdr = hdr;

            vmem_header_t *hole = (void *)new_ftr + sizeof(vmem_footer_t);
            hole->magic = VMEM_HEAP_MAGIC;
            hole->hole = 1;
            hole->size = (uintptr_t)rftr + sizeof(vmem_footer_t) - (uintptr_t)hole;
            hole->align = 0;

            rftr->hdr = hole;

            ord_arr_insert(&heap->index, hole);
            return old;
        } else {
            /* no new hole fits, so claim the entire hole */
            hdr->size += rhdr->size;
            rftr->hdr = hdr;
            return old;
        }
    }

    /* no way to extend our allocation means we need a new one */
    void *new_alloc = vmem_heap_alloc(heap, size - sizeof(vmem_header_t)
                                                 - sizeof(vmem_footer_t),
                                      hdr->align);
    memcpy(new_alloc, old, hdr->size - sizeof(vmem_header_t)
                                     - sizeof(vmem_footer_t));
    vmem_heap_free(heap, old);
    return new_alloc;
}

void vmem_heap_free(vmem_heap_t *heap, void *p) {
    if(p == NULL) return;

    vmem_header_t *hdr = p - sizeof(vmem_header_t);
    vmem_footer_t *ftr = (void *)hdr + hdr->size - sizeof(vmem_footer_t);

    assert(hdr->magic == VMEM_HEAP_MAGIC);
    assert(ftr->magic == VMEM_HEAP_MAGIC);
    assert(hdr->hole == 0 && "Double free");

    hdr->hole = 1;
    hdr->align = 0;
    bool add_hole = true;

    /* unify left */
    vmem_footer_t *lftr = (void *)hdr - sizeof(vmem_footer_t);
    if(lftr->magic == VMEM_HEAP_MAGIC && lftr->hdr->hole) {
        lftr->hdr->size += hdr->size;
        hdr = lftr->hdr;
        ftr->hdr = hdr;
        add_hole = false; /* already in index */
    }

    /* unify right */
    vmem_header_t *rhdr = (void *)ftr + sizeof(vmem_footer_t);
    if(rhdr->magic == VMEM_HEAP_MAGIC && rhdr->hole) {
        hdr->size += rhdr->size;
        ftr = (void *)rhdr + rhdr->size - sizeof(vmem_footer_t);
        ftr->hdr = hdr;

        size_t idx = 0;
        while(idx < heap->index.size && ord_arr_get(&heap->index, idx) != rhdr)
            idx++;

        /* make sure it actually exists */
        assert(idx < heap->index.size);
        ord_arr_remove(&heap->index, idx);
    }

    /* TODO: contract */

    if(add_hole) ord_arr_insert(&heap->index, hdr);
}

/* dump information about heap segments */
void vmem_heap_dump(vmem_heap_t *heap) {
    vmem_header_t *hdr = heap->start;

    while((void *)hdr < heap->end) {
        if(hdr->magic != VMEM_HEAP_MAGIC) goto magic_err;
        vmem_footer_t *ftr = (void *)hdr + hdr->size - sizeof(vmem_footer_t);
        if(ftr->magic != VMEM_HEAP_MAGIC) goto magic_err;
        printf("%p\talloc:%d sz:%#zx\trsz:%#zx\n",
               hdr, !hdr->hole,
               hdr->size - sizeof(vmem_header_t) - sizeof(vmem_footer_t),
               hdr->size);
        hdr = (void *)hdr + hdr->size;
    }
    return;

magic_err:
    printf("\033[41mSegment at %p not intact!\033[m\n", hdr);
}

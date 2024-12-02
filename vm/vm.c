/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/vaddr.h"'
#include "vm/uninit.h"
#include "threads/mmu.h"
#include "lib/kernel/hash.h"
#include <string.h>


/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`.
 * 커널이 새 페이지 요청 받으면 호출되는 함수
 * 페이지 구조체를 할당하여 새로운 페이지 initialize
 * 페이지 타입에 맞추고 user program에게 control back */

bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	//  1은 anonymous, 2는 filebacked
	
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		/* TODO: Insert the page into the spt. */
		/* uninit_new에 맞는 함수 매개 변수 선언해야 함 
			여기에서 원하는 것은 uninit page 생성 후 spt에 삽입*/

		// 1. 페이지 할당 
		struct page * page = (struct page*)malloc(sizeof(struct page));

		// 2. 함수 포인터  
		bool (*initializer)(struct page *, enum vm_type, void *);

		// type에 따라 초기화 함수 정의
		if(VM_TYPE(type) == VM_ANON){
			initializer = anon_initializer;
		}else if(type == VM_FILE){
			initializer = file_backed_initializer;
		 }
		
		uninit_new (page, upage, init, type, aux, initializer);

		page->writable = writable; 

		return spt_insert_page(spt,page);

	}

err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct hash *h = &spt->spt_hash;

	// 페이지 할당  - page va 대입 - hash find 함수 - return hash_elem
	struct page *p = (struct page *)malloc(sizeof(struct page));
	p->va = pg_round_down(va);
	struct hash_elem *e = hash_find (h, &p->hash_elem);
	free(p);

	if(e != NULL){
		return hash_entry(e, struct page, hash_elem);
	}
	return NULL; 
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED, struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	// page->hash_elem을 spt->hash에 삽입
	if (!hash_insert(&spt->spt_hash,&page->hash_elem)){
		succ = true;
		return succ;
	}
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.
 * Gets a new physical page from the user pool by calling palloc_get_page.
 * When successfully got a page from the user pool, also allocates a frame,initialize its members, and returns it.
 * After you implement vm_get_frame, you have to allocate all user space pages (PALLOC_USER) through this function.
 * You don't need to handle swap out for now in case of page allocation failure. Just mark those case with PANIC ("todo") for now.
*/
static struct frame *
vm_get_frame(void)
{
    // 프레임 구조체 할당
    struct frame *frame = malloc(sizeof(struct frame));
    ASSERT(frame != NULL);

    frame->kva = palloc_get_page(PAL_USER | PAL_ZERO);

    if (frame->kva == NULL)
        frame = vm_evict_frame();


    frame->page = NULL; // 초기에는 연결된 페이지가 없음

    ASSERT(frame->page == NULL);

    return frame; // 초기화된 프레임 반환
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED, bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	// printf("🚨 Address: %p\n", addr);

	if(not_present){
		//  사실상 이 부분이 핵심 (페이지를 구해야 하기 떄문에)
		page = spt_find_page(spt,addr);
		if(page == NULL){
			return false;
		}
		// if(write==1 && page->writable == 0){
		// 	return false ;
		// }
		return vm_do_claim_page (page);
	}
	
	return false ;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	struct thread *t = thread_current();
	page = spt_find_page(&t->spt, va);

	/*
	pml4_get_page 같은 경우, 이미 매핑된 va를 pml4 테이블에서 찾는 것
	spt_find_page 같은 경우, 해당 프로세스에 관리 중인 가상 페이지에 대한 정보가 있고
	여기에서 아직 매핑되지 않는 va를 찾는다. 
	두 테이블의 정보 개수 비교 : SPT ≥ PML4
	*/
	
	if(page == NULL){
		return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
/* Claims, meaning allocate a physical frame, a page.
 * You first get a frame by calling vm_get_frame (which is already done for you in the template).
 * Then, you need to set up the MMU.
 * In other words, add the mapping from the virtual address to the physical address in the page table.
 * The return value should indicate whether the operation was successful or not.
 */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();


	// if (!frame || !frame->kva) {
    // PANIC("🚨 Invalid frame returned by vm_get_frame!");
	// }
	// if (!page || !page->va) {
    // PANIC("🚨 Invalid page structure!");
	// }	
	// if (page == NULL || frame == NULL) {
	// 	PANIC("🚨 NULL pointer exception at page or frame!");
	// }


	if(frame == NULL){
		return false;
	}

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	// * 페이지 테이블의 실제 주소에 가상 주소의 매핑

	struct thread *t = thread_current();
	bool success = pml4_set_page(t->pml4,page->va,frame->kva, page->writable);
	
	if (success){
		return swap_in (page, frame->kva);
	}else{
		palloc_free_page(frame->kva);
		free(frame);
		return false;
	}	
	return false;
	
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->spt_hash,hash_func,less_func,NULL);
}

/* Copy supplemental page table from src to dst */
// 이 함수 같은 경우 자식 프로세스가 실행 중이라는 전재
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {

	struct hash_iterator *i; 
	hash_first(i,&src->spt_hash);

	while(hash_next(i)){
		struct page *src_page = hash_entry(hash_cur(i), struct page, hash_elem);
		void * upage = src_page->va;
		bool writable = src_page->writable;

		// vm_alloc_page_with_initializer로 새로운 페이지 할당 및 spt의 엔트리 추가 
		// vm_type이 UNINT 인 경우 
		enum vm_type type = src_page->operations->type;
		if(type == VM_UNINIT){
			vm_initializer *init = (&src_page->uninit)->init;
			void *aux = (&src_page->uninit)->aux;
			vm_alloc_page_with_initializer(VM_ANON | VM_MARKER_0, upage, writable, init, aux);
		}

		// vm_type이 UNINT 이외의 경우
		if (!vm_alloc_page_with_initializer(type,upage,writable,NULL,NULL)){
			return false ; 	
		}

		// VA를 PA와 매핑
		if(!vm_claim_page(upage)){
			return false ;
		}

		struct page *dst_page = spt_find_page(dst,upage);
		memcpy(dst_page->frame->kva,src_page->frame->kva,PGSIZE);
	}

	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}




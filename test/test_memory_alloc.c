
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "pthread.h"

#include "np_memory.h"
#include "np_log.h"
#include "neuropil.h"
#include "message.h"
#include "include.h"

typedef struct test_struct
{
		unsigned int i_test;
		char* s_test;
} test_struct_t;


void test_struct_t_del(void* data_ptr) {
	printf("destructor test_struct_t_del called");
}
void test_struct_t_new(void* data_ptr) {
	printf("constructor test_struct_t_new called");
}

int main(int argc, char **argv) {

	int level = LOG_ERROR | LOG_WARN | LOG_INFO | LOG_DEBUG;
	log_init("./test_memory_alloc.log", level);

	np_printpool;

	np_obj_t *np_obj1, *np_obj2, *np_obj3, *np_obj4;
	test_struct_t *t_obj1, *t_obj2, *t_obj3, *t_obj4;

	{
		np_new(test_struct_t, np_obj1);
		np_bind(test_struct_t, np_obj1, t_obj1);

		t_obj1->i_test = 1;
		t_obj1->s_test = "dies ist ein test";
	}

	np_printpool;

	{
		np_new(test_struct_t, np_obj2);
		np_bind(test_struct_t, np_obj2, t_obj2);

		t_obj2->i_test = 2;
		t_obj2->s_test = "dies ist zwei test";
	}
	np_printpool;

	{
		np_new(test_struct_t, np_obj3);
		np_bind(test_struct_t, np_obj3, t_obj3);

		t_obj3->i_test = 3;
		t_obj3->s_test = "dies ist drei test";
	}
	np_printpool;

	np_unbind(test_struct_t, np_obj3, t_obj3);

	np_printpool;

	{
		np_new(test_struct_t, np_obj4);
		np_bind(test_struct_t, np_obj4, t_obj4);

		t_obj4->i_test = 4;
		t_obj4->s_test = "dies ist vier test";
		// np_ref(test_struct_t, t_obj);
	}

	np_printpool;

	np_unbind(test_struct_t, np_obj1, t_obj1);

	np_printpool;

	np_unbind(test_struct_t, np_obj1, t_obj1);


	np_state_t* state = np_init(3000);
	np_msginterest_t* interested = np_message_create_interest(state, "test", ONE_WAY, 1, 1);

	const char* test_msg = "test.message";
	for (int i =0; i < 100; i++)
		np_msgcache_push(interested, new_val_v(test_msg));

	printf("size of msgcache %d\n", np_msgcache_size(interested));
	for (int i =0; i < 50; i++) {
		np_val_t message = np_msgcache_pop(interested);
	}
	printf("size of msgcache %d\n", np_msgcache_size(interested));

	for (int i =0; i < 50; i++) {
		np_val_t message = np_msgcache_pop(interested);
	}
	printf("size of msgcache %d\n", np_msgcache_size(interested));
}

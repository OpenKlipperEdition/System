#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "event_client.h"


#define EVT_CLIENT_NAME1      "evtClient1"
#define EVT_CLIENT_NAME2      "evtClient2"
#define EVT_CLIENT_NAME3      "evtClient3"

void testSend1(evt_msg_t evt_msg)
{
	struct input_event *iev = NULL;

	iev = (struct input_event *)evt_msg.evt_msg;

	printf("----- [%s] : iev.type = %d\t iev.code = %d\t iev.value = %d\n", \
			EVT_CLIENT_NAME1, iev->type, iev->code, iev->value);
}

void testSend2(evt_msg_t evt_msg)
{
	struct input_event *iev = NULL;

	iev = (struct input_event *)evt_msg.evt_msg;

	printf("+++++ [%s] : iev.type = %d\t iev.code = %d\t iev.value = %d\n", \
			EVT_CLIENT_NAME2, iev->type, iev->code, iev->value);
}

void testSend3(evt_msg_t evt_msg)
{
	struct input_event *iev = NULL;

	iev = (struct input_event *)evt_msg.evt_msg;

	printf("===== [%s] : iev.type = %d\t iev.code = %d\t iev.value = %d\n", \
			EVT_CLIENT_NAME3, iev->type, iev->code, iev->value);
}

int main(int argc, char **argv)
{
	int ret = 0;

	if (2 != argc)
		return -1;

	evt_clt_t *p_evt_clt = NULL;

	if (0 == strcmp("1", argv[1])) {
		/* 测试不取消监听直接注销事件客户端 */
		p_evt_clt = create_evt_clt(EVT_CLIENT_NAME1);
		ret = register_evt_clt(p_evt_clt);
		ret = listen_event(p_evt_clt, KEY_EVENT, testSend1);

		printf("----- [%s] : listening key event.......\n", EVT_CLIENT_NAME1);
		sleep(20);
		printf("----- unregister_evt_clt[%s] ...\n", EVT_CLIENT_NAME1);
	} else if (0 == strcmp("2", argv[1])) {
		/* 测试取消监听正常注销事件客户端 */
		p_evt_clt = create_evt_clt(EVT_CLIENT_NAME2);
		ret = register_evt_clt(p_evt_clt);
		ret = listen_event(p_evt_clt, KEY_EVENT, testSend2);

		printf("+++++ [%s] : listening key event.......\n", EVT_CLIENT_NAME2);
		sleep(20);
		printf("sleep exit ...........................................\n");

		ret = cancel_listen_event(p_evt_clt, KEY_EVENT);
	} else if (0 == strcmp("3", argv[1])) {
		/* 测试监听按键事件（作为一个进程可和其他进程配合测试） */
		p_evt_clt = create_evt_clt(EVT_CLIENT_NAME3);
		ret = register_evt_clt(p_evt_clt);
		ret = listen_event(p_evt_clt, KEY_EVENT, testSend3);

		printf("===== [%s] : listening key event.......\n", EVT_CLIENT_NAME3);
			int cnt = 20;
			while(cnt--){
				sleep(1);
			}

		ret = cancel_listen_event(p_evt_clt, KEY_EVENT);
		goto end;
	} else {
		printf("Parameter error.\n");
		return -1;
	}

end:
	ret = unregister_evt_clt(p_evt_clt);
	destroy_evt_clt(p_evt_clt);

    binder_threads_shutdown();
	return 0;
}

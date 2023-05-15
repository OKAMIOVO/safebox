// #define KEY_SET_PORT	PORT1
// #define KEY_SET_PIN		PIN0
// #define KEY_MODE_PORT	PORT2
// #define KEY_MODE_PIN	PIN1
// #define CLOSE_TEST_PORT	PORT2
// #define CLOSE_TEST_PIN	PIN3
// #define VIBRATION_TEST_PORT PORT13
// #define VIBRATION_TEST_PIN	PIN6
// MultiTimer keyTimer;
// void BeepInit()
//{
//     TM41_SquareOutput(TM4_CHANNEL_3, 1000);
// }
//  enum{
//  	KEY_ASTER=10,
//  	KEY_POUND,
//	KEY_MODE,
//	KEY_SET
//  };
// void KeyTimerHandler(MultiTimer* timer,void* userData)
//{
//		if(!PORT_GetBit(KEY_SET_PORT,KEY_SET_PIN))
//		{
//			printf("key set\n");
//		}
//		if(!PORT_GetBit(KEY_MODE_PORT,KEY_MODE_PIN))
//		{
//			printf("key mode\n");
//		}
//		if(!PORT_GetBit(CLOSE_TEST_PORT,CLOSE_TEST_PIN))
//		{
//			printf("not closes\n");
//		}
//		if(!PORT_GetBit(VIBRATION_TEST_PORT,VIBRATION_TEST_PIN))
//		{
//			printf("vibration test\n");
//		}
//		MultiTimerStart(&keyTimer,100,KeyTimerHandler,NULL);
// }
// void KeyInit()
//{
//	PORT_Init(KEY_SET_PORT,KEY_SET_PIN,INPUT);
//	PORT_Init(KEY_MODE_PORT,KEY_MODE_PIN,INPUT);
//	PORT_Init(CLOSE_TEST_PORT,CLOSE_TEST_PIN,INPUT);
////	INTP_Init(1 << 0,INTP_FALLING);
//	//PORT_Init(VIBRATION_TEST_PORT,VIBRATION_TEST_PIN,INPUT);
//	printf("Key ini\n");
//	MultiTimerStart(&keyTimer,100,KeyTimerHandler,NULL);
//
//}
//	KeyInit();
// BeepInit();
// TM41_SquareOutput(TM4_CHANNEL_3, 1000);
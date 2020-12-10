#include<stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include<string.h>
#pragma warning(disable:4996)
typedef void(*ptr_func_v2)(uint8_t *p_buf);//数据处理函数的指针
#define CRC8_INIT     0x55
#define CRC16_INIT    0x8624
#define V2_SOF  0x64

#define  MODULE_SOC          0x01
#define  MODULE_MCU          0x02
#define  MODULE_PC           0x03
#define  MODULE_APP          0x04
#define  MODULE_SERVER       0x0A
#define  MODULE_DEFAULT      0x1F


#define  FLAG_SOC            (0x01<<MODULE_SOC)
#define  FLAG_MCU            (0x01<<MODULE_MCU)
#define  FLAG_PC             (0x01<<MODULE_PC)
#define  FLAG_APP            (0x01<<MODULE_APP)
#define  FLAG_SERVER         (0x01<<MODULE_SERVER)

#define  HEADER_LEN      sizeof( cmd_header_v2_t )
/* 一个包最小的长度 包头+CRC16 两个字节 */
#define  MINIMUM_LEN     (HEADER_LEN + 2)


//header  Attributes  define
#define   ENCRYPT_TYPE_NONE  0x00
#define   ENCRYPT_TYPE_DES   0x01

#define   CMD_TYPE_REQ       0x00
#define   CMD_TYPE_ACK       0x01

#define   ACK_TYPE_NONE      0x00
#define   ACK_TYPE_IMMEDIATE 0x01
#define   ACK_TYPE_LATER     0x02


//cmd set
#define   CMD_SET_COMMON     0x00
#define   CMD_SET_SOC        0x01
#define   CMD_SET_MCU        0x02
#define   CMD_SET_PC         0x03
#define   CMD_SET_APP        0x04
#define   CMD_SET_SERVER     0x0A
#pragma pack(push)
#pragma pack(1)
/**************************
 *
 * header 数据包头结构体
 *
 **************************/
typedef  struct {
    uint8_t sof;
    struct {
        uint16_t length        : 12;
        uint16_t version       : 4;
    } v2;
    uint8_t crc8;
    struct {
        uint8_t tx_id    : 5;
        uint8_t tx_index : 3;
    } tx;
    struct {
        uint8_t rx_id    : 5;
        uint8_t rx_index : 3;
    } rx;
    struct {
        uint8_t encrypt_type  : 4;
        uint8_t cmd_req_ack   : 1;
        uint8_t cmd_ack_type  : 2;
        uint8_t reserve       : 1;
    } attri;
    uint8_t set;
    uint8_t id;
    uint16_t seqnum;
} cmd_header_v2_t;
/**************************
 *
 * 解包结构体
 *
 **************************/
typedef struct {
    uint8_t     step;//解包步骤
    uint16_t    idx;//数据包字节索引
    uint16_t    len;//当前数据包的长度
    uint16_t    max;//当前可接收数据的最大长度
    uint8_t     ver;//版本号
    uint8_t     *p_data;//完整数据包存放位置
} v2_decoder_object_t;

typedef struct {
    uint8_t            set;
    uint8_t            id;
    ptr_func_v2        handler;
}handler_v2_t;

/*********************************************
 * id : 0x01
 * des: 查询版本
 *********************************************/
typedef  struct {
    uint8_t  result;
    uint8_t  hardware_id[16];
    uint32_t firmware_ver;
    uint32_t loader_ver;
    uint32_t command_set;
    uint8_t  hardware_version;
} cmd_device_info_ack_t;



//the max python file size?????
#define MAX_FILE_SIZE  65535
/*********************************************
 * id : 0x30
 * des: start file transmission
 *********************************************/
typedef  struct {
    uint8_t encrypt;
    uint32_t file_size;
    uint16_t filename_length;
    char filename[32];
} cmd_start_file_trasmission_req_t;

typedef  struct {
    uint8_t result;
    uint16_t package_size;
} cmd_start_file_trasmission_ack_t;


/*********************************************
 * id : 0x31
 * des: transmit file data
 *********************************************/
typedef  struct {
    uint8_t encrypt;
    int32_t package_index;
    uint16_t package_length;
    uint8_t data[1024];
} cmd_file_data_req_t;

typedef  struct {
    uint8_t result;
    uint32_t package_index;
} cmd_file_data_ack_t;


/*********************************************
 * id : 0x32
 * des: finish file transmission
 *********************************************/
typedef  struct {
    uint8_t encrypt;
    uint8_t md5[16];
} cmd_finish_file_transmission_req_t;

typedef  struct {
    uint8_t result;
} cmd_finish_file_transmission_ack_t;


/*********************************************
 * id : 0x33
 * des: start video transmission
 *********************************************/
typedef  struct {
    uint8_t encrypt;
    uint32_t image_size;
    uint16_t image_name_length;
    char image_name[20];
} cmd_start_video_transmission_req_t;


/*********************************************
 * id : 0x34
 * des: transmit video
 *********************************************/
typedef  struct {
    uint8_t encrypt;
    uint32_t package_index;
    uint16_t package_length;
    uint8_t data[1024];
} cmd_transmit_video_data_req_t;


/*********************************************
 * id : 0x03
 * des: stop the python program
 *********************************************/
typedef  struct {
    uint8_t    cmd;
} cmd_stop_python_req_t;

typedef  struct {
    uint8_t    result;
} cmd_stop_python_ack_t;


/*********************************************
 * id : 0x04
 * des: python code finished
 *********************************************/
typedef  struct {
    uint8_t  cmd;
} cmd_python_finished_req_t;

typedef  struct {
    uint8_t  result;
} cmd_python_finished_ack_t;

/*********************************************
 * id : 0x04
 * des: 设置速度及角速度
*********************************************/
typedef  struct {
    float  speed;
    float  theta;
} cmd_set_speed_req_t;

typedef  struct {
    uint8_t  result;
} cmd_set_speed_ack_t;


/*********************************************
 * id : 0x05
 * des: 设置飞轮正反停
 *飞轮转速范围为-100~+100，表示飞轮的转速百分比。
 * 如果值大于0表示顺时针旋转转，
 * 如果小于0表示逆时针旋转
*********************************************/
typedef  struct {
    uint8_t  speed;
} cmd_set_fly_wheel_req_t;

typedef  struct {
    uint8_t  result;
} cmd_set_fly_wheel_ack_t;


/*********************************************
 * id : 0x06
 * des: 设置螺杆正反停
 *      0x00:正
 *      0x08:停
 *      0x0f:反
*********************************************/
typedef  struct {
    uint8_t  command;
} cmd_set_screw_req_t;

typedef  struct {
    uint8_t  result;
} cmd_set_screw_ack_t;
#pragma pack(pop)

void test(uint8_t *data);
unsigned char get_crc8_check_sum(unsigned char *msg,unsigned int cnt,unsigned char crc8);
unsigned int verify_crc8_check_sum(unsigned char *msg, unsigned int length);
void append_crc8_check_sum(unsigned char *msg, unsigned int length);


unsigned short get_crc16_check_sum( unsigned char *msg, unsigned int length, unsigned short crc16);
unsigned int verify_crc16_check_sum( unsigned char *msg, unsigned int length );
void append_crc16_check_sum( unsigned char *msg, unsigned int length );




//void v2_procotol_init( v2_port_t *port, uint8_t *decoder_buf, uint16_t decoder_max ,uint8_t *rx_buf );
void v2_protocol_send(uint8_t *data);
void v2_protocol_process(v2_decoder_object_t* obj,uint8_t data);
void v2_protocol_init_ack_and_send( uint8_t *p_buf , uint16_t data_len );
void v2_protocol_init_req_and_send( uint8_t rx , uint8_t set , uint8_t id ,uint8_t need_ack , uint8_t *p_buf , uint16_t data_len );


void cmd_common_start_python_file_transmission_req( char *file_name , uint32_t file_size);//python发送开始命令
void cmd_common_transmite_python_file_req(uint8_t *p_buf, uint32_t index, uint16_t length, uint32_t receive_length);//python代码数据发送命令
void cmd_common_transmite_python_file_complete_req( );//python代码发送完成的命令

//处理收到的回包函数接口
void cmd_common_start_python_file_transmission_ack(uint8_t*p_buf);//
void cmd_common_transmite_python_file_ack(uint8_t*p_buf);
void cmd_common_transmite_python_file_complete_ack(uint8_t*p_buf);


//发送python代码函数接口
void sendPythonCode(uint8_t*code, int len);
void cmd_common_start_video_receive_ack(uint8_t *p_buf);
void cmd_common_tranmite_video_data_ack(uint8_t *p_buf);
bool image_data_receive(uint8_t*image_data,uint8_t*data, uint32_t pack_length);





void  cmd_stop_python_ack(uint8_t *p_buf);
//void  cmd_push_python_finished_status(void );
void  cmd_python_finished_status_ack(uint8_t *p_buf);

void  cmd_stop_python_req();//终止python代码运行




void  cmd_mcu_set_fly_wheel_req(uint8_t speed);
void  cmd_mcu_set_fly_wheel_ack(uint8_t *p_buf );
void  cmd_mcu_set_speed_req(float speed , float theta);
void  cmd_mcu_set_brake_req(uint8_t cmd);
void  cmd_mcu_set_brake_ack(uint8_t *p_buf );













#include"proto.h"

#define MY_MODULE  MODULE_APP
extern char file_path[64];
FILE * fp_file = NULL;
uint32_t count = 0;//记录图像数据包的数目
uint32_t last_package_index = -1;
uint32_t file_size = 0;
uint32_t bytes_write = 0;//记录图像字节数
uint32_t file_index = 0;
int index_number = 0;

uint32_t image_size = 0;//每次接收到的图像大小
uint8_t image_data[65536];//此处需要根据每次图像的大小初始化，接收图像数
uint8_t *code_data=NULL;//已知
uint32_t code_file_length;//已知
int temp_length = 0;

int   tail_pack_size = 0;
int total_pack_number = 0;
//处理回包的函数接口，需要修改**************************************************************************************************
const handler_v2_t cmd_handler_table_v2[] = {
        { CMD_SET_COMMON , 0x30, cmd_common_start_python_file_transmission_ack },//python文件头发送状态查询
        { CMD_SET_COMMON , 0x31, cmd_common_transmite_python_file_ack },//python文件数据发送状态查询
        { CMD_SET_COMMON , 0x32, cmd_common_transmite_python_file_complete_ack },//python文件发送结束状态查询
        { CMD_SET_COMMON , 0x33 , cmd_common_start_video_receive_ack },
        { CMD_SET_COMMON , 0x34 , cmd_common_tranmite_video_data_ack },
        { CMD_SET_SOC	 , 0x03 , cmd_stop_python_ack },//终止python程序运行的状态查询
        { CMD_SET_SOC    , 0x04 , cmd_python_finished_status_ack },//python运行状态查询，python程序正常运行结束后会发送该命令
        { 0 , 0 , NULL },
};
const unsigned char CRC8_TAB[256] =
        {
                0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
                0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
                0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
                0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
                0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
                0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
                0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
                0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
                0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
                0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
                0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
                0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
                0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
                0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
                0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
                0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
        };


// caculate crc value by lookup table
unsigned char get_crc8_check_sum(unsigned char *msg,unsigned int cnt,unsigned char crc8)
{
    unsigned char idx;

    while (cnt--)
    {
        idx = crc8^(*msg++);
        crc8  = CRC8_TAB[idx];
    }

    return(crc8);
}

/*
 * 整条信息的最后一个字节是CRC8校验，判断在最后这个字节是不是对的
*/
unsigned int verify_crc8_check_sum(unsigned char *msg, unsigned int length)
{
    unsigned char expected = 0;

    if ((msg == NULL) || (length <= 2)) return 0;

    expected = get_crc8_check_sum (msg, length-1, CRC8_INIT);

    return ( expected == msg[length-1] );
}

/*
 * 整条信息的最后一个字节是CRC8校验，算好后插在最后这个字节
*/
void append_crc8_check_sum(unsigned char *msg, unsigned int length)
{
    if ((msg == NULL) || (length <= 2)) return;

    msg[length-1] = get_crc8_check_sum (msg, length-1, CRC8_INIT);
}

void test(uint8_t *data)
{
    cmd_file_data_req_t*p_ack = (cmd_file_data_req_t*)(data + HEADER_LEN);
    printf("code=%s", p_ack->data);

}


const unsigned short wCRC_Table[256] = {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

/*
**  Descriptions: CRC16 checksum function
**  Input:        Data to check,Stream length, initialized checksum
**  Output:       CRC checksum
*/
unsigned short get_crc16_check_sum( unsigned char *msg, unsigned int length, unsigned short crc16 )
{
    unsigned char chData = 0;
    if( msg == NULL ) {
        return 0xFFFF;
    }

    while( length-- ) {
        chData = *msg++;
        crc16 = (crc16 >> 8)  ^ wCRC_Table[(crc16 ^ (unsigned short)(chData)) & 0x00ff];
    }

    return crc16;
}

unsigned int verify_crc16_check_sum( unsigned char *msg, unsigned int length )
{
    unsigned short expected = 0;

    if( ( msg == NULL ) || ( length <= 2 ) ) {
        return 0xFFFF;
    }
    expected = get_crc16_check_sum( msg, length - 2, CRC16_INIT );

    return ( ( expected & 0xff ) == msg[length - 2] ) && ( ( ( expected >> 8 ) & 0xff ) == msg[length - 1] );
}


void append_crc16_check_sum( unsigned char *msg, unsigned int length )
{
    unsigned short data = 0;

    if ((msg == NULL) || (length <= 2))
    {
        return;
    }
    data = get_crc16_check_sum (msg, length-2, CRC16_INIT );

    msg[length-2] = ( unsigned char )( data & 0x00ff );
    msg[length-1] = ( unsigned char )( ( data >> 8 ) & 0x00ff );
}


/*********************************************
 * 解码V2协议
 *********************************************/
bool v2_decoder( v2_decoder_object_t *obj, uint8_t data )
{
    switch( obj->step ) {
        case 0:
            if( data == V2_SOF ) {
                obj->idx = 0;
                obj->p_data[obj->idx++] = data;
                obj->step = 1;
                printf("step 0..............\n");
            }
            break;
        case 1:
            obj->len = data;
            obj->p_data[obj->idx++] = data;
            obj->step = 2;
            printf("step 1..............\n");
            break;
        case 2:
            printf("step 2..............\n");
            obj->len |= ( ( data & 0x0f ) << 8 );
            obj->ver = ( data >> 4 );
            if( obj->len > obj->max || obj->ver != 0x02) {
                obj->step = 0;
                printf("step 2.....a.........%d\n", obj->ver);
            } else {
                obj->p_data[obj->idx++] = data;
                obj->step = 3;
                printf("step 2.......b.......\n");
            }
            break;
        case 3:
            printf("step 3..............\n");
            obj->p_data[obj->idx++] = data;
            if( verify_crc8_check_sum( obj->p_data, 4 ) ) {
                obj->step = 4;
            } else {
                obj->step = 0;
            }
            break;
        case 4:
            printf("step 4..............\n");
            if( obj->idx < obj->len ) {
                obj->p_data[obj->idx++] = data;
            }
            if( obj->idx == obj->len ) {
                obj->step = 0;
                if( verify_crc16_check_sum( obj->p_data, obj->len ) ) {
                    printf("step 4.........123.....\n");
                    return 1 ;
                }
            }else if( obj->idx > obj->len ){
                obj->step = 0;
            }
            break;
        default:
            obj->step = 0;
    }
    return 0;
}

/*********************************************
 *
 * 初始化回包的格式
 *********************************************/
void v2_protocol_init_ack_and_send( uint8_t *p_buf , uint16_t data_len )
{
    cmd_header_v2_t *p_cmd = ( cmd_header_v2_t * )p_buf;

    uint8_t tmp0 = p_cmd->tx.tx_index;
    uint8_t tmp1 = p_cmd->tx.tx_id;

    p_cmd->tx.tx_index = p_cmd->rx.rx_index;
    p_cmd->tx.tx_id = p_cmd->rx.rx_id;
    p_cmd->rx.rx_index = tmp0;
    p_cmd->rx.rx_id = tmp1;

    p_cmd->attri.cmd_req_ack = CMD_TYPE_ACK;
    p_cmd->attri.cmd_ack_type = ACK_TYPE_NONE;

    p_cmd->v2.length = MINIMUM_LEN + data_len;

    v2_protocol_send( p_buf );
}


/*********************************************
 * 初始化请求包格式
 *********************************************/
void v2_protocol_init_req_and_send( uint8_t rx , uint8_t set , uint8_t id ,uint8_t need_ack , uint8_t *p_buf , uint16_t data_len )
{
    cmd_header_v2_t *p_cmd = ( cmd_header_v2_t * )p_buf;
    static uint16_t seq_num = 0;

    p_cmd->sof = V2_SOF;
    p_cmd->v2.version = 0x02;

    p_cmd->tx.tx_index = 0;
    p_cmd->tx.tx_id = MY_MODULE;
    p_cmd->rx.rx_index = 0;
    p_cmd->rx.rx_id = rx;

    p_cmd->attri.cmd_ack_type = need_ack; // 应答类型：不需要  立即  稍后
    p_cmd->attri.cmd_req_ack = CMD_TYPE_REQ;  //请求包，有可能是推送 有可能是请求，看上面是否需要应答
    p_cmd->attri.encrypt_type = ENCRYPT_TYPE_NONE;  //加密类型为无
    p_cmd->attri.reserve = 0;
    //printf("set=%d\n", set);
    p_cmd->seqnum = seq_num++;
    p_cmd->set = set;
    p_cmd->id  = id;
    p_cmd->v2.length = MINIMUM_LEN + data_len;

    v2_protocol_send( p_buf );

}





/*********************************************
* 根据接收者自动选择发送端口并发送
 *********************************************/
void v2_protocol_send(uint8_t *data)
{
    cmd_header_v2_t* p_cmd = (cmd_header_v2_t*)data;
    append_crc8_check_sum( data , 4 );
    append_crc16_check_sum( data , p_cmd->v2.length );

}


/*********************************************
 * 主循环调用该函数进行解码
 *********************************************/
void v2_protocol_process(v2_decoder_object_t* obj,uint8_t data)
{
    uint8_t ii;
    cmd_header_v2_t* p_cmd;

    if( v2_decoder(obj, data) == 1){

        p_cmd = (cmd_header_v2_t*)obj->p_data;
        if( p_cmd->rx.rx_id != MY_MODULE && p_cmd->rx.rx_id != MODULE_DEFAULT ){
            // v2_protocol_send(obj->p_data);
            test(obj->p_data);
            //printf("***********************parse successqqq*********************%d***%d*\n", p_cmd->id, p_cmd->set);

        }else{
            printf("***********************parse success*********************%d***%d*\n", p_cmd->id, p_cmd->set);
            for(ii=0 ; cmd_handler_table_v2[ii].handler != NULL ; ii++){
                if( p_cmd->id == cmd_handler_table_v2[ii].id && p_cmd->set == cmd_handler_table_v2[ii].set ){

                    cmd_handler_table_v2[ii].handler(obj->p_data);
                    break;
                }
            }
        }
    }
}

uint32_t pre_package_length = 0;//记录前一个图像数据包的长度
bool image_data_receive(uint8_t*image_data,uint8_t*data,uint32_t pack_length)
{
    bool flag = false;


    memcpy(image_data + pre_package_length*count, data,pack_length);//将每次接收到的图像数据包填到image_data中，直到填满完整图像
    count++;//记录图像数据传输帧，每次传输1024字节
    bytes_write += pack_length;
    pre_package_length = pack_length;
    if (bytes_write == image_size)
    {
        //一帧完整的图像接收完毕
        flag = true;
        bytes_write = 0;
        count = 0;
    }
    return flag;







}

/*********************************************
 * id : 0x33
 * des: start video receive
 *********************************************/

void cmd_common_start_video_receive_ack( uint8_t *p_buf )//接收图像头数据,目的为了获取图像的大小image_size
{
    uint8_t result = 0;
    cmd_header_v2_t *p_cmd = ( cmd_header_v2_t * )p_buf;
    cmd_start_video_transmission_req_t *p_req = (cmd_start_video_transmission_req_t * )( p_buf + HEADER_LEN );

    //printf("cmd_common_start_file_transmission_ack received file size %d  %x %x %x %x\n", p_req->file_size, *(p_buf + HEADER_LEN+1), *(p_buf + HEADER_LEN+2), *(p_buf + HEADER_LEN+3), *(p_buf + HEADER_LEN+4));

    if( p_req->encrypt != 0 ) return;

    if( p_req->image_size > MAX_FILE_SIZE ){
        result = 0xF7;  //文件过大
    }else{
        image_size = p_req->image_size;
        printf("cmd_common_start_video_receive_ack received file name lenght %d \n", p_req->image_name_length);

        char file_name[32] = {0};
        memcpy(file_name, p_buf + HEADER_LEN + sizeof(cmd_start_file_trasmission_req_t), p_req->image_name_length);

        printf("filename = %s,len = %d\n",file_name, p_req->image_name_length);

    }

}


/*********************************************
 * id : 0x34
 * des: rx video data
 *********************************************/
void cmd_common_tranmite_video_data_ack( uint8_t *p_buf )
{
    uint8_t result = 0;
    int32_t temp_index ;

    cmd_header_v2_t *p_cmd = ( cmd_header_v2_t * )p_buf;
    cmd_transmit_video_data_req_t *p_req = (cmd_transmit_video_data_req_t * )( p_buf + HEADER_LEN );

    //printf("cmd_common_tranmite_video_data_ack received index %d  length:%d\n", p_req->package_index, p_req->package_length);
    if( p_req->encrypt != 0 ) return;
    //memcpy(video_data+pre_package_length*temp_index,p_req->data,p_req->package_length);
    image_data_receive(image_data, p_req->data, p_req->package_length);
}

/*********************************************
 * id : 0x30
 * des: start python file transmission
 *********************************************/
void cmd_common_start_python_file_transmission_req( char *file_name , uint32_t file_size)//发送文件名和文件大小
{
    uint8_t req_buf[MINIMUM_LEN + sizeof(cmd_start_file_trasmission_req_t)];
    cmd_start_file_trasmission_req_t *p_push = (cmd_start_file_trasmission_req_t * )(req_buf + HEADER_LEN);
    //设置信息
    p_push->encrypt = 0;
    p_push->file_size = file_size;
    p_push->filename_length = strlen((char*)file_name);
    memcpy(p_push->filename, file_name, strlen((char*)file_name));
    //printf("p_push->filename=%s\n", p_push->filename);

    v2_protocol_init_req_and_send(MODULE_SOC , CMD_SET_COMMON , 0x30 , ACK_TYPE_IMMEDIATE , req_buf , sizeof(cmd_start_file_trasmission_req_t));
}

/*********************************************
 * id : 0x31
 * des: transmit python file  data
 *********************************************/
void cmd_common_transmite_python_file_req( uint8_t *p_buf , uint32_t index, uint16_t length,uint32_t receive_length)//发送python文件数据
{
    uint8_t req_buf[MINIMUM_LEN + sizeof(cmd_file_data_req_t)];
    cmd_file_data_req_t *p_push = (cmd_file_data_req_t *)(req_buf + HEADER_LEN);
    //设置信息
    p_push->encrypt = 0;
    p_push->package_index = index;
    p_push->package_length = length;
    memcpy(p_push->data, code_data + index*receive_length, length);
    v2_protocol_init_req_and_send(MODULE_SOC , CMD_SET_COMMON , 0x31 , ACK_TYPE_IMMEDIATE , req_buf , sizeof(cmd_file_data_req_t));
    printf("sizeof(cmd_file_data_req_t)=%d\n", sizeof(cmd_file_data_req_t));



}

/*********************************************
 * id : 0x32
 * des: transmit python file completed
 *********************************************/
void cmd_common_transmite_python_file_complete_req(  )//发送python文件数据结束命令，md5可以先不用管，直接用一个16字节长度的16进制序列代替例
//[94, 182, 59, 187, 224, 30, 238, 208, 147, 203, 34, 187, 143, 90, 205, 195]
{
    uint8_t req_buf[MINIMUM_LEN + sizeof(cmd_finish_file_transmission_req_t)];
    cmd_finish_file_transmission_req_t *p_push = ( cmd_finish_file_transmission_req_t * )(req_buf + HEADER_LEN);
    unsigned char temp_md5[16]={4, 182, 59, 187, 224, 30, 238, 208, 147, 203, 34, 187, 143, 90, 205, 195 };
    //设置信息
    p_push->encrypt = 0;
    memcpy(p_push->md5, temp_md5, 16);
    v2_protocol_init_req_and_send(MODULE_SOC , CMD_SET_COMMON , 0x32 , ACK_TYPE_IMMEDIATE , req_buf , sizeof(cmd_finish_file_transmission_req_t));
}


void cmd_common_start_python_file_transmission_ack(uint8_t*p_buf)
{
    cmd_start_file_trasmission_ack_t*p_ack = (cmd_start_file_trasmission_ack_t*)(p_buf + HEADER_LEN);
    //
    p_ack->package_size;//每次需要传输的字节数
    p_ack->result;//接收到的回包的状态值，非零为失败
    if (p_ack->result != 0)
    {
        index_number= 0;
        return;
    }

    //开始传输第一帧
    if (p_ack->package_size <= 0)
    {
        return;
    }
    temp_length = p_ack->package_size;//每次以此长度发送数据
    tail_pack_size = code_file_length%temp_length;
    if (tail_pack_size == 0)
        total_pack_number = code_file_length / temp_length;
    else
        total_pack_number = code_file_length / temp_length + 1;

    if (total_pack_number == 1)
    {
        cmd_common_transmite_python_file_req(code_data, index_number, code_file_length, code_file_length);

    }
    else {
        // Log.e("MyLog","fuck.............1..............");
        cmd_common_transmite_python_file_req(code_data, index_number, temp_length, temp_length);
        //ndFileData(index_number, file_data, temp_length, temp_length);
    }



    //d_common_transmite_python_file_req(code_data, 0, p_ack->package_size);



}
void cmd_common_transmite_python_file_ack(uint8_t*p_buf)
{
    cmd_file_data_ack_t*p_ack = (cmd_file_data_ack_t*)(p_buf + HEADER_LEN);
    //
    p_ack->package_index;//接收到的回包的索引
    p_ack->result;//同上
    if (!p_ack->result) return;

    if (index_number + 1 == total_pack_number)
    {

        //python文件传输完成命令

        cmd_common_transmite_python_file_complete_req();
        index_number = 0;
        return;

    }
    index_number++;
    if (tail_pack_size == 0)
    {
        //发送数据
//sendFileData(index_number, file_data, temp_length, temp_length);

        cmd_common_transmite_python_file_req(code_data, index_number, temp_length, temp_length);

    }
    else
    {
        if (index_number == total_pack_number - 1)
        {
            // Log.e("MyLog","fuck..........2................."+"length= "+temp_length);
//endFileData(index_number, file_data, tail_pack_size, temp_length);
            cmd_common_transmite_python_file_req(code_data, index_number, tail_pack_size, temp_length);


        }
        else
        {
            //sendFileData(index_number, file_data, temp_length, temp_length);
            cmd_common_transmite_python_file_req(code_data, index_number, temp_length, temp_length);
        }
    }



}
void cmd_common_transmite_python_file_complete_ack(uint8_t*p_buf)
{
    cmd_finish_file_transmission_ack_t*p_ack = (cmd_finish_file_transmission_ack_t*)(p_buf + HEADER_LEN);
    //
    p_ack->result;//同上
    if (p_ack->result != 0)
    {
        return;
    }
    index_number = 0;
    printf("receive transmite_python_file_complete cmd\n");

}
void sendPythonCode(uint8_t*code, int len)//code为python文件数据字节，len为python文件数据的长度
{
    code_data = code;
    cmd_common_start_python_file_transmission_req("maego.py", len);//发送python文件开始传输命令

}

/*********************************************
 * id : 0x03
 * des: stop python code   send
 *********************************************/
void  cmd_stop_python_req()
{
    uint8_t req_buf[MINIMUM_LEN + sizeof(cmd_stop_python_req_t)];
    req_buf[HEADER_LEN] = 0x01;
    v2_protocol_init_req_and_send(MODULE_SOC , CMD_SET_COMMON , 0x03 , ACK_TYPE_IMMEDIATE , req_buf , sizeof(cmd_stop_python_req_t));
}

/*********************************************
* id : 0x03
* des: push python code stop status to pc or app
*********************************************/
void  cmd_stop_python_ack(uint8_t *p_buf)
{
    uint8_t req_buf[MINIMUM_LEN + sizeof(cmd_stop_python_ack_t)];
    cmd_stop_python_ack_t *p_ack = ( cmd_stop_python_ack_t * )( req_buf + HEADER_LEN );
    printf("cmd_push_Stop_python__status!\n");
}



/*********************************************
 * id : 0x04
 * des: push python code finished status to pc or app
 *********************************************/
void  cmd_python_finished_status_ack(uint8_t *p_buf)
{
    //uint8_t req_buf[MINIMUM_LEN + sizeof(cmd_python_finished_req_t)];
    ///=----------------------------

    cmd_python_finished_ack_t *p_ack = ( cmd_python_finished_ack_t * )( p_buf + HEADER_LEN );

    printf("cmd_push_python_finished_status  ack received!\n");
}


/*********************************************
 * id : 0x04
 * des: 设置速度及角速度  7002发
 *     @speed  速度    m/s
 *     @theta  角速度  rad/s
 *********************************************/
void  cmd_mcu_set_speed_req(float speed , float theta)
{
    uint8_t req_buf[MINIMUM_LEN + sizeof(cmd_set_speed_req_t)];
    cmd_set_speed_req_t *p_req = (cmd_set_speed_req_t *)(req_buf + HEADER_LEN);

    p_req->speed = speed;
    p_req->theta = theta;

    v2_protocol_init_req_and_send(MODULE_MCU , CMD_SET_MCU , 0x04 , ACK_TYPE_IMMEDIATE , req_buf , sizeof(cmd_set_speed_req_t));
}


/*********************************************
 * id : 0x05
 * des: 设置飞轮正反停
 *飞轮转速范围为-100~+100，表示飞轮的转速百分比。
 * 如果值大于0表示顺时针旋转转，
 * 如果小于0表示逆时针旋转
*********************************************/
void  cmd_mcu_set_fly_wheel_req(uint8_t speed)//设为95,不需要接收返回状态7002发
{
    uint8_t req_buf[MINIMUM_LEN + sizeof(cmd_set_fly_wheel_req_t)];
    cmd_set_fly_wheel_req_t *p_req = (cmd_set_fly_wheel_req_t *)(req_buf + HEADER_LEN);

    p_req->speed = speed;

    v2_protocol_init_req_and_send(MODULE_MCU , CMD_SET_MCU , 0x05 , ACK_TYPE_IMMEDIATE , req_buf , sizeof(cmd_set_speed_req_t));
}

void  cmd_mcu_set_fly_wheel_ack(uint8_t *p_buf )
{
    uint8_t result = 0;
    cmd_header_v2_t *p_cmd = ( cmd_header_v2_t * )p_buf;
    cmd_set_fly_wheel_req_t *p_req = ( cmd_set_fly_wheel_req_t * )( p_buf + HEADER_LEN );
    cmd_set_fly_wheel_ack_t *p_ack = ( cmd_set_fly_wheel_ack_t * )( p_buf + HEADER_LEN );;

    //do sth
    p_req->speed = p_req->speed;

    if( p_cmd->attri.cmd_ack_type != ACK_TYPE_NONE ) {

        p_ack->result = result;
        v2_protocol_init_ack_and_send(p_buf,sizeof( cmd_set_fly_wheel_ack_t ));
    }
}

/*********************************************
 * id : 0x06
 * des: 设置刹车
 *0x00：刹车
 *0x08：停止
 *0x0F：释放
*********************************************/
void  cmd_mcu_set_brake_req(uint8_t cmd)//设为0x00，不需要接收返回状态7002发
{
    uint8_t req_buf[MINIMUM_LEN + sizeof(cmd_set_screw_req_t)];
    cmd_set_screw_req_t *p_req = (cmd_set_screw_req_t *)(req_buf + HEADER_LEN);

    p_req->command = cmd;

    v2_protocol_init_req_and_send(MODULE_MCU , CMD_SET_MCU , 0x06 , ACK_TYPE_IMMEDIATE , req_buf , sizeof(cmd_set_screw_req_t));
}

void  cmd_mcu_set_brake_ack(uint8_t *p_buf )
{
    uint8_t result = 0;
    cmd_header_v2_t *p_cmd = ( cmd_header_v2_t * )p_buf;
    cmd_set_screw_req_t *p_req = ( cmd_set_screw_req_t * )( p_buf + HEADER_LEN );
    cmd_set_screw_ack_t *p_ack = ( cmd_set_screw_ack_t * )( p_buf + HEADER_LEN );;

    //do sth
    printf("get ack of cmd_mcu_set_brake_req\n");

    if( p_cmd->attri.cmd_ack_type != ACK_TYPE_NONE ) {

        p_ack->result = result;
        v2_protocol_init_ack_and_send(p_buf,sizeof( cmd_set_screw_ack_t ));
    } else {
    }
}


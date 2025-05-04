#define         table_index_t           int8_t

#define         UWB_COMMUNICATION_SEND_POSITION_ENABLE

// ADHOCDECK_H
#define         UWB_MAX_TIMESTAMP       1099511627776

// TABLE_LINK_LIST_H
#define         TABLE_BUFFER_SIZE       10      
#define         FREE_QUEUE_SIZE         TABLE_BUFFER_SIZE

// RANGING_BUFFER_H
#define         RANGING_BUFFER_SIZE     6
#define         CONVERGENCE_THRESHOLD   0.7     // 收敛阈值
#define         VELOCITY                0.4691763978616

// MODIFIED_RANGING_H
#define         MESSAGE_BODY_RX_SIZE    1
#define         MESSAGE_HEAD_TX_SIZE    1
#define         MESSAGE_BODY_UNIT_SIZE  1
#define         TX_BUFFER_POOL_SIZE     TABLE_BUFFER_SIZE   
#define         TABLE_SET_NEIGHBOR_NUM  10
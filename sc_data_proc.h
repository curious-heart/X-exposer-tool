#ifndef SC_DATA_PROC_H
#define SC_DATA_PROC_H

#include <QByteArray>

#define RECV_DATA_NOTE_E(e) e

#define RECV_DATA_NOTES \
    RECV_DATA_NOTE_E(NORMAL), \
    RECV_DATA_NOTE_E(START_ACK), \
    RECV_DATA_NOTE_E(STOP_ACK), \
    RECV_DATA_NOTE_E(UNEXPECTED_IN_START_WAIT), \
    RECV_DATA_NOTE_E(UNEXPECTED_IN_STOP_WAIT), \
    RECV_DATA_NOTE_E(IRRELEVANT_ADDR), \
    RECV_DATA_NOTE_E(RECV_IN_IDLE),

typedef enum
{
    RECV_DATA_NOTES
}recv_data_notes_e_t;

typedef struct
{
    recv_data_notes_e_t  notes;
    QByteArray data;
}recv_data_with_notes_s_t;

#endif // SC_DATA_PROC_H

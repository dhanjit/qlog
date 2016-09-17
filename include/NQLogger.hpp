#ifndef _NQ_LOGGER_HPP_
#define _NQ_LOGGER_HPP_

#include <FprintfSyncLogger.hpp>
#include <FstreamSyncLogger.hpp>
#include <MultiLogger.hpp>
#include <MultiQueueAsyncLogger.hpp>
#include <SpscAsyncLogger.hpp>

namespace common {
namespace logger {
using initlog_t = MultiLogger<FprintfSyncLogger &, FprintfSyncLogger, FullLevelList, ThresholdLevelList<level::ERROR>>;
}
}

#define _PASS_

#if defined(_LOGGER_SETUP_SPSC_)
#define MAINLOG(level, tag, args...) this->mainLog.log<common::logger::label::LabelList<level, SCT(tag)>>(MicroSecondTime{}, ##args)
#define SENDLOG(level, tag, args...) this->sendLog.log<common::logger::label::LabelList<level, SCT(tag)>>(args)
#define RECVLOG(level, tag, args...) this->recvLog.log<common::logger::label::LabelList<level, SCT(tag)>>(args)
#define UPDTLOG(level, tag, args...) this->updtLog.log<common::logger::label::LabelList<level, SCT(tag)>>(args)
#define INITLOG(level, tag, args...) initLog.log<common::logger::label::LabelList<level, SCT(tag)>>(this->timeNow, ##args)
#elif defined(_LOGGER_SETUP_MQSC_)
#define MAINLOG(level, tag, args...) this->mainLog.log<common::logger::label::LabelList<level, SCT(tag)>>(MicroSecondTime{}, ##args)
#define SENDLOG(level, tag, args...) this->log.log<common::logger::label::LabelList<level, SCT("[SEND]"), SCT(tag)>, common::logger::QId<0>>(args)
#define RECVLOG(level, tag, args...) this->log.log<common::logger::label::LabelList<level, SCT("[RECV]"), SCT(tag)>, common::logger::QId<1>>(args)
#define UPDTLOG(level, tag, args...) this->log.log<common::logger::label::LabelList<level, SCT("[UPDT]"), SCT(tag)>, common::logger::QId<2>>(args)
#define INITLOG(level, tag, args...) initLog.log<common::logger::label::LabelList<level, SCT(tag)>>(this->timeNow, ##args)
#elif defined(_LOGGER_SETUP_MPSC_)
#define MAINLOG(level, tag, args...) this->mainLog.log<common::logger::label::LabelList<level, SCT(tag)>>(MicroSecondTime{}, ##args)
#define SENDLOG(level, tag, args...) this->log.log<common::logger::label::LabelList<level, SCT("[SEND]"), SCT(tag)>, common::logger::QId<0>>(args)
#define RECVLOG(level, tag, args...) this->log.log<common::logger::label::LabelList<level, SCT("[RECV]"), SCT(tag)>, common::logger::QId<1>>(args)
#define UPDTLOG(level, tag, args...) this->log.log<common::logger::label::LabelList<level, SCT("[UPDT]"), SCT(tag)>, common::logger::QId<2>>(args)
#define INITLOG(level, tag, args...) initLog.log<common::logger::label::LabelList<level, SCT(tag)>>(this->timeNow, ##args)
#else
#error message("Logger Setup Error")
#endif

// #define RECVLOG(type, args...) _PASS_//void(0)this->recvlog.log<type>(__VA_ARGS__);
// #define UPDTLOG(type, args...) _PASS_//this->updtlog.log<type>(__VA_ARGS__);

/**************
  OMD thread:
 **************/

// Debug
#ifdef _NQ_LOG_DEBUG_
#define NQ_LOG_DEBUG(args...) SENDLOG(common::logger::level::DEBUG, ##args)
#else
#define NQ_LOG_DEBUG(args...) _PASS_
#endif

// Info
#ifdef _NQ_LOG_INFO_
#define NQ_LOG_INFO(args...) SENDLOG(common::logger::level::INFO, ##args)
// _C for continue line.
//#define NQ_LOG_INFO_C(args...) SENDLOG(common::logger::level::INFO, ##args)
#else
#define NQ_LOG_INFO(args...) _PASS_
#endif

#ifdef _NQ_LOG_SENT_
#define NQ_LOG_SENT(args...) NQ_LOG_INFO(args)    // Sent
#else
#define NQ_LOG_SENT(args...) _PASS_
#endif

#define NQ_LOG_WARN(args...) SENDLOG(common::logger::level::WARN, ##args)      // Warn
#define NQ_LOG_ERROR(args...) SENDLOG(common::logger::level::ERROR, ##args)    // Error
#define NQ_LOG_CRIT(args...) SENDLOG(common::logger::level::CRIT, ##args)      // Critical Errror

/**************
  Recv thread
****************/

#ifdef _NQ_LOG_INFO_
#define NQ_LOG_RECV_INFO(args...) RECVLOG(common::logger::level::INFO, ##args)
#else
#define NQ_LOG_RECV_INFO(args...) _PASS_
#endif

#define NQ_LOG_RECV_WARN(args...) RECVLOG(common::logger::level::WARN, ##args)
#define NQ_LOG_RECV_ERROR(args...) RECVLOG(common::logger::level::ERROR, ##args)
#define NQ_LOG_RECV_CRIT(args...) RECVLOG(common::logger::level::CRIT, ##args)

/*****************
  Update thread
*******************/
#define NQ_LOG_UPDT_INFO(args...) UPDTLOG(common::logger::level::INFO, ##args)
#define NQ_LOG_UPDT_WARN(args...) UPDTLOG(common::logger::level::WARN, ##args)
#define NQ_LOG_UPDT_ERROR(args...) UPDTLOG(common::logger::level::ERROR, ##args)
#define NQ_LOG_UPDT_CRIT(args...) UPDTLOG(common::logger::level::CRIT, ##args)

/*****************
  Main log
*******************/
#ifdef _NQ_LOG_INFO_
#define NQ_LOG_MAIN_INFO(args...) MAINLOG(common::logger::level::INFO, ##args)
#else
#define NQ_LOG_MAIN_INFO(args...) _PASS_
#endif

#define NQ_LOG_MAIN_WARN(args...) MAINLOG(common::logger::level::WARN, ##args)
#define NQ_LOG_MAIN_ERROR(args...) MAINLOG(common::logger::level::ERROR, ##args)
#define NQ_LOG_MAIN_CRIT(args...) MAINLOG(common::logger::level::CRIT, ##args)

/*****************
  Init log
*******************/
#ifdef _NQ_LOG_INFO_
#define NQ_LOG_INIT_INFO(args...) INITLOG(common::logger::level::INFO, ##args)
#else
#define NQ_LOG_INIT_INFO(args...) _PASS
#endif

#define NQ_LOG_INIT_WARN(args...) INITLOG(common::logger::level::WARN, ##args)
#define NQ_LOG_INIT_ERROR(args...) INITLOG(common::logger::level::ERROR, ##args)
#define NQ_LOG_INIT_CRIT(args...)                 \
    INITLOG(common::logger::level::CRIT, ##args); \
    initLog.flush()

// Formatting
#define _FF(x, y) \
    common::logger::FormattedValue<decltype(x), y> { x }
#define _FN(x, y, z) \
    common::logger::FormattedValue<decltype(x), y, z> { x }

namespace common {
namespace logger {
static constexpr char sidechar[4] = {'A', 'B', 'S', '0'};
inline static const char &getSide(irage::enOrdSide side) { return sidechar[static_cast<int>(side)]; }
}
}

#define NQ_SIDE(x) common::logger::getSide(x)
#define NOVALUE(x) common::logger::PlaceHolder

#ifdef _IRZ_INCLUDE_NSE_T_
#define NQ_EXCHANGE "NSE_T"
#elif defined _IRZ_INCLUDE_NSE_
#define NQ_EXCHANGE "NSE"
#else
#define NQ_EXCHANGE NOVALUE("Exchange")
#endif

#endif

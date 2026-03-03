#ifndef SC_QUOTA_H
#define SC_QUOTA_H

enum ApiQuotaStatus
{
  API_QUOTA_OK = 0,
  API_QUOTA_REACHED,
  API_QUOTA_TRACKING_ERROR
};

enum ApiQuotaStatus consume_daily_api_quota (void);

#endif

#ifndef QUERY_DATABASE_H
#define QUERY_DATABASE_H

#include "ESPSupabase.h"
#include <string>

class QueryDatabase
{
private:
    ESPSupabase supabase;
    access_token token;
public:
    void checkSupabaseConnection(String url, String key);
    void queryData(const std::string &query);
    void sendData(const std::string &query);
}
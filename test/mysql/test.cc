#include <iostream>
#include <vector>
#include <algorithm>
#include "ngx_db.h"

using namespace std;

unsigned int MurmurHash2(const void *key, int len, unsigned int seed)
{
    // http://cn.voidcc.com/question/p-qvepvvth-bkh.html
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.

    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value

    unsigned int h = seed ^ len;

    // Mix 4 bytes at a time into the hash

    const unsigned char *data = (const unsigned char *)key;

    while (len >= 4)
    {
        unsigned int k = *(unsigned int *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array

    switch (len)
    {
    case 3:
        h ^= data[2] << 16;
    case 2:
        h ^= data[1] << 8;
    case 1:
        h ^= data[0];
        h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

void test1()
{
    MyDB db;
    //连接数据库
    db.initDB("106.52.58.96", "apple", "star", "test");
    //将用户信息添加到数据库
    db.exeSQL("INSERT accounts values('fengxin','123');");
    db.exeSQL("INSERT accounts values('axin','456');");
    //将所有用户信息读出，并输出。
    db.exeSQL("SELECT * from accounts;");
}

string to_62(unsigned int x)
{
    vector<char> cs{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    string result;
    while (x != 0)
    {
        int r = x % 62;
        // int r=x-(x/62)*62;
        result.push_back(cs[r]);
        x /= 62;
    }
    reverse(result.begin(), result.end());
    return result;
}

void test2()
{
    // int x = MurmurHash2("test",12,97);

    int len = 6;
    MyDB db;
    //连接数据库
    if (!db.initDB("106.52.58.96", "apple", "star", "test"))
        cout << "MySQL连接失败" << endl;

    // string sql = "INSERT INTO short_url_map (lurl, hash,gmt_create) VALUES ('now()','1234',CURRENT_TIMESTAMP(2));";
    // db.exeSQL(sql);

    db.exeSQL("SELECT * from short_url_map;");
    while (1)
    {
        string s;

        cin >> s;
        if (s == "q")
            break;
        auto x = MurmurHash2(s.c_str(), len, 97);
        cout << x << "     ";
        string hash = to_62(x);
        cout << hash << endl;
        // --------------------------------

        //将用户信息添加到数据库
        string sql = "INSERT INTO short_url_map (lurl, hash) VALUES ('" + s + "', '" + hash + "');";
        // string sql = "INSERT INTO short_url_map (lurl, hash,gmt_create) VALUES ('qaq','1234','now()');";
        db.exeSQL(sql);
        // db.exeSQL("INSERT accounts values('axin','456');");
        //将所有用户信息读出，并输出。
        db.exeSQL("SELECT * from short_url_map;");
    }
    string x = "good";
    // string sql = "SELECT lurl FROM short_url_map where hash='" + x + "';";
    string sql = "SELECT hash FROM short_url_map where lurl='" + x + "';";
    if (db.exeSQL(sql))
        cout << "查询成功" << endl;
}
int main()
{
    test2();
    return 0;
}

#include <iostream>
#include <string>
#include <mysql/mysql.h>
using namespace std;

class MyDB
{
    // 类中套类，用于自动释放
    class AutoRelease
    {
    public:
        ~AutoRelease()
        {
            if (m_instance)
            {
                // cout << "~AutoRelease:"<< (int*)m_instance << endl;
                delete m_instance;
            }
        }
    };

public:
    static MyDB *GetInstance()
    {
        if (m_instance == nullptr)
        {
            static AutoRelease ar;
            m_instance = new MyDB();
        }
        return m_instance;
    }
    ~MyDB();
    bool initDB(string host, string user, string pwd, string db_name); //连接mysql
    bool exeSQL(string sql);                                           //执行sql语句
    string find_surl_SQL(string lurl);                                 //根据lurl找出surl
    string find_lurl_SQL(string surl);                                 //根据surl找出lurl
    bool add_surl_SQL(string surl, string lurl);                                    //添加一个短网址
private:
    static MyDB *m_instance; // 单例模式实例指针
    MyDB();
    MYSQL *mysql;      //连接mysql句柄指针
    MYSQL_RES *result; //指向查询结果的指针
    MYSQL_ROW row;     //按行返回的查询信息
};

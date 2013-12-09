#ifndef DB_H
#define DB_H

#include <mongo/client/connpool.h>
#include "Log.h"
#include <string>
#include <utility>

namespace mongo
{

/** \brief Класс-обёртка над MongoDB.

    Этот класс нужен для упрощения доступа к базам данных MongoDB. Во-первых,
    вводится понятие "текущей базы данных", как это сделано, например, в
    python или javascript драйверах mongo. Во-вторых, все подключения
    сосредотачиваются в одном месте.

    Все подключения регистрируются с помощью статического метода addDatabase.
    Существует одно безымянное подключение "по умолчанию" и произвольное
    количество именованных подключений. Получить именованный экземпляр можно
    передав в конструктор имя базы данных.

    Например:

    // В начале программы регистрируем и инициализируем необходимые подключения
    DB::addDatabase("localhost", "getmyad_db");   // безымянное подключение
    DB::addDatabase("db2", "localhost", "data2"); // подключение с именем "db2"

    // Теперь в любом месте программы можем использовать подключения:
    DB db;		    // Подключение по умолчанию
    DB db2("db2");	    // Именованное подключение
    default.findOne("collection", QUERY());
    db2.remove("trash", QUERY());


    Возможно также подключаться к кластеру баз данных (Replica Set). Для этого
    нужно использовать версии методов addDatabase, принимающие в качестве
    параметра класс ReplicaSetConnection.

 */
class DB
{
public:
    DB(const std::string &name = std::string())
        : db_( 0 )
    {
        options_ = options_by_name(name);
        if (!options_)
            throw NotRegistered("Database " +
                                (name.empty()? "(default)" : name) +
                                " is not registered! Use addDatabase() first.");

        if (options_->replica_set.empty())
            db_ = ScopedDbConnection::getScopedDbConnection(options_->server_host);
        else
            db_ = ScopedDbConnection::getScopedDbConnection(
                      ConnectionString(ConnectionString::SET,
                                       options_->server_host,
                                       options_->replica_set));
    }

    ~DB()
    {
        db_->done();
        delete db_;
    }

    /** Описывает строку подключения к MongoDB Replica Set. */
    class ReplicaSetConnection
    {
        std::string replica_set_;
        std::string connection_string_;
    public:
        /** Создаёт строку подключения к MongoDB Replica Set.

            \a replica_set -- название replica_set
            \a connection_string -- строка подключения вида
        			    "server1[:port1],server2[:port2],..." */
        ReplicaSetConnection(const std::string &replica_set,
                             const std::string &connection_string)
            : replica_set_(replica_set),
              connection_string_(connection_string) { }

        std::string replica_set() const
        {
            return replica_set_;
        }
        std::string connection_string() const
        {
            return connection_string_;
        }
    };

    /** Регистрирует подключение \a name к базе данных.

    Строка подключения передаётся в параметре \a server_host и имеет вид
    "server[:port]".

    После регистрации можно	получать подключение с этими параметрами через
    конструктор (см. описание класса). */
    static void addDatabase(const std::string &name,
                            const std::string &server_host,
                            const std::string &database,
                            bool slave_ok)
    {
        _addDatabase(name, server_host, database, "", slave_ok);
    }

    /** Регистрирует подключение \a name к набору реплик баз данных
    (Replica Set). */
    static void addDatabase(const std::string &name,
                            const ReplicaSetConnection &connection_string,
                            const std::string &database,
                            bool slave_ok)
    {
        _addDatabase(name, connection_string.connection_string(), database,
                     connection_string.replica_set(), slave_ok);
    }

    /** Регистрирует базу данных "по умолчанию". */
    static void addDatabase(const std::string &server_host,
                            const std::string &database,
                            bool slave_ok)
    {
        _addDatabase("", server_host, database, "", slave_ok);
    }

    /** Регистрирует базу данных "по умолчанию", подключение осуществляется
    к набору реплик (Replica Set). */
    static void addDatabase(const ReplicaSetConnection &connection_string,
                            const std::string &database, bool slave_ok)
    {
        _addDatabase("", connection_string.connection_string(), database,
                     connection_string.replica_set(), slave_ok);
    }

    /** Возвращает полное название коллекции */
    std::string collection(const std::string &collection)
    {
        return database() +  "." + collection;
    }

    /** Название используемой базы данных */
    std::string &database()
    {
        return options_->database;
    }

    /** Адрес сервера MongoDB */
    std::string &server_host()
    {
        return options_->server_host;
    }

    /** Название набора реплик (replica set) */
    std::string &replica_set()
    {
        return options_->replica_set;
    }

    /** Возвращает true, если допускается read-only подключение к slave серверу в кластере */
    bool slave_ok()
    {
        return options_->slave_ok;
    }

    /** Возвращает соединение к базе данных.
    Может использоваться для операций, не предусмотренных обёрткой.
     */
    ScopedDbConnection &db()
    {
        return *db_;
    }


    /** Вспомогательный метод, возвращающий значение поля field как int.
        Реально значение может храниться как в int, так и в string.
        Если ни одно преобразование не сработало, вернёт 0.
     */
    static int toInt(const BSONElement &element)
    {
        switch (element.type())
        {
        case NumberInt:
            return element.numberInt();
        case String:
            try
            {
                return boost::lexical_cast<int>(element.str());
            }
            catch (boost::bad_lexical_cast &)
            {
                return 0;
            }
        default:
            return 0;
        }
    }

    /** Вспомогательный метод, возвращающий значение поля field как float.
        Реально значение может храниться как в int или double, так и в string.
        Если ни одно преобразование не сработало, вернёт 0.

    	добавлено RealInvest Soft
     */
    static float toFloat(const BSONElement &element)
    {
        switch (element.type())
        {
        case NumberInt:
            return (float)element.numberInt();
        case NumberDouble:
            return (float)element.numberDouble();
        case String:
            try
            {
                return boost::lexical_cast<float>(element.str());
            }
            catch (boost::bad_lexical_cast &)
            {
                return 0;
            }
        default:
            return 0;
        }
    }




    // Все нижеследующие методы являются просто обёртками над методами
    // DBClientConnection, принимающие вместо параметра ns (namespace)
    // параметр coll (collection) и автоматически добавляющие к названию
    // коллекции имя базы данных.
    //
    // Некоторые методы (insert, update, remove) принимают также
    // дополнительный параметр safe, который, если равен true, заставляет
    // ждать ответа от базы данных, таким образом гарантируя выполнение
    // операции (особенность mongodb в асинхронном выполнении команд,
    // т.е. по умолчанию метод завершится сразу, а реальное изменение
    // данных может произойти позже)
    //
    // Если при создании подключения был указан параметр slave_ok = true,
    // то функции чтения (query, findOne, count) всегда будут устанавливать
    // бит QueryOption_SlaveOk.

    void insert( const string &coll, BSONObj obj, bool safe = false )
    {
        (*db_)->insert(this->collection(coll), obj);
        if (safe)
            (*db_)->getLastError();
    }

    void remove( const string &coll, Query obj, bool justOne = 0,
                 bool safe = false)
    {
        (*db_)->remove(this->collection(coll), obj, justOne);
        if (safe)
            (*db_)->getLastError();
    }

    void update( const string &coll, Query query, BSONObj obj,
                 bool upsert = 0, bool multi = 0, bool safe = false )
    {
        (*db_)->update(this->collection(coll), query, obj, upsert, multi);
        if (safe)
            (*db_)->getLastError();
    }

    BSONObj findOne(const string &coll, Query query,
                    const BSONObj *fieldsToReturn = 0, int queryOptions = 0)
    {
        if (options_->slave_ok)
            queryOptions |= QueryOption_SlaveOk;
        return (*db_)->findOne(this->collection(coll), query, fieldsToReturn,
                               queryOptions);
    }

    std::unique_ptr<DBClientCursor> query(const string &coll, Query query,
                                          int nToReturn = 0, int nToSkip = 0,
                                          const BSONObj *fieldsToReturn = 0,
                                          int queryOptions = 0, int batchSize = 0 )
    {
        if (options_->slave_ok)
            queryOptions |= QueryOption_SlaveOk;
        return unique_ptr<DBClientCursor>(
                   (*db_)->query(this->collection(coll), query, nToReturn, nToSkip,
                                 fieldsToReturn, queryOptions, batchSize).release());
    }

    unsigned long long count(const string &coll,
                             const BSONObj& query = BSONObj(), int options=0 )
    {
        if (options_->slave_ok)
            options |= QueryOption_SlaveOk;
        return (*db_)->count(this->collection(coll), query, options);
    }

    bool createCollection(const string &coll, long long size = 0,
                          bool capped = false, int max = 0, BSONObj *info = 0)
    {
        return (*db_)->createCollection(this->collection(coll), size,
                                        capped, max, info);
    }

    bool dropCollection( const string &coll )
    {
        return (*db_)->dropCollection(this->collection(coll));
    }

    bool dropDatabase()
    {
        return (*db_)->dropDatabase(database());
    }


    /** Исключение, возникающее при попытке обратиться к незарегистированной
    базе данных */
    class NotRegistered : public std::exception
    {
    public:
        NotRegistered(const std::string &str) : error_message(str) { }
        NotRegistered(const char *str) : error_message(str) { }
        virtual ~NotRegistered() throw() { }
        virtual const char *what() const throw()
        {
            return error_message.c_str();
        }
    private:
        std::string error_message;
    };

private:
    ScopedDbConnection *db_;


protected:

    struct ConnectionOptions
    {
        ConnectionOptions() : slave_ok(false) {}
        std::string database;
        std::string server_host;
        std::string replica_set;
        bool slave_ok;
    } *options_;

    typedef std::map<std::string, ConnectionOptions*> ConnectionOptionsMap;
    static ConnectionOptionsMap connection_options_map_;

    /** Возвращает настройки для базы данных с именем \a name.
    Если база данных с таким именем не была добавлена, вернёт 0. */
    static ConnectionOptions *options_by_name(const std::string &name)
    {
        ConnectionOptionsMap::const_iterator it =
            connection_options_map_.find(name);
        if (it != connection_options_map_.end())
            return it->second;
        else
            return 0;
    }

    /** Добавляет настройки подключения */
    static void _addDatabase(const std::string &name,
                             const std::string &server_host,
                             const std::string &database,
                             const std::string &replica_set,
                             bool slave_ok)
    {
        ConnectionOptions *options = options_by_name(name);
        if (!options)
            options = new ConnectionOptions;
        else
        {
            Log::warn("Database %s is already registered. Old connection will be overwritten.",
                       (name.empty()? "(default)" : name).c_str());
        };

        options->database = database;
        options->server_host = server_host;
        options->replica_set = replica_set;
        options->slave_ok = slave_ok;
        connection_options_map_[name] = options;
    }
};




}  // end namespace mongo
#endif // DB_H

#include "DB.h"

mongo::DB::ConnectionOptionsMap mongo::DB::connection_options_map_;

mongo::DB::DB(const std::string &name)
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

mongo::DB::~DB()
{
    db_->done();
    delete db_;
}

void mongo::DB::addDatabase(const std::string &name,
                            const std::string &server_host,
                            const std::string &database,
                            bool slave_ok)
{
    _addDatabase(name, server_host, database, "", slave_ok);
}

/** Регистрирует подключение \a name к набору реплик баз данных
(Replica Set). */
void mongo::DB::addDatabase(const std::string &name,
                            const ReplicaSetConnection &connection_string,
                            const std::string &database,
                            bool slave_ok)
{
    _addDatabase(name, connection_string.connection_string(), database,
                 connection_string.replica_set(), slave_ok);
}

/** Регистрирует базу данных "по умолчанию". */
void mongo::DB::addDatabase(const std::string &server_host,
                            const std::string &database,
                            bool slave_ok)
{
    _addDatabase("", server_host, database, "", slave_ok);
}

/** Регистрирует базу данных "по умолчанию", подключение осуществляется
к набору реплик (Replica Set). */
void mongo::DB::addDatabase(const ReplicaSetConnection &connection_string,
                            const std::string &database, bool slave_ok)
{
    _addDatabase("", connection_string.connection_string(), database,
                 connection_string.replica_set(), slave_ok);
}

/** Возвращает полное название коллекции */
std::string mongo::DB::collection(const std::string &collection)
{
    return database() +  "." + collection;
}

/** Название используемой базы данных */
std::string &mongo::DB::database()
{
    return options_->database;
}

/** Адрес сервера MongoDB */
std::string &mongo::DB::server_host()
{
    return options_->server_host;
}

/** Название набора реплик (replica set) */
std::string &mongo::DB::replica_set()
{
    return options_->replica_set;
}

/** Возвращает true, если допускается read-only подключение к slave серверу в кластере */
bool mongo::DB::slave_ok()
{
    return options_->slave_ok;
}

/** Возвращает соединение к базе данных.
Может использоваться для операций, не предусмотренных обёрткой.
 */
mongo::ScopedDbConnection &mongo::DB::db()
{
    return *db_;
}


/** Вспомогательный метод, возвращающий значение поля field как int.
    Реально значение может храниться как в int, так и в string.
    Если ни одно преобразование не сработало, вернёт 0.
 */
int mongo::DB::toInt(const BSONElement &element)
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
float mongo::DB::toFloat(const BSONElement &element)
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

void mongo::DB::insert( const std::string &coll, BSONObj obj, bool safe )
{
    (*db_)->insert(this->collection(coll), obj);
    if (safe)
        (*db_)->getLastError();
}

void mongo::DB::remove( const std::string &coll, Query obj, bool justOne,
                        bool safe )
{
    (*db_)->remove(this->collection(coll), obj, justOne);
    if (safe)
        (*db_)->getLastError();
}

void mongo::DB::update( const std::string &coll, Query query, mongo::BSONObj obj,
                        bool upsert, bool multi, bool safe)
{
    (*db_)->update(this->collection(coll), query, obj, upsert, multi);
    if (safe)
        (*db_)->getLastError();
}

mongo::BSONObj mongo::DB::findOne(const std::string &coll, Query query,
                                  const mongo::BSONObj *fieldsToReturn, int queryOptions)
{
    if (options_->slave_ok)
        queryOptions |= QueryOption_SlaveOk;
    return (*db_)->findOne(this->collection(coll), query, fieldsToReturn,
                           queryOptions);
}

std::unique_ptr<mongo::DBClientCursor> mongo::DB::query(const std::string &coll, Query query,
        int nToReturn, int nToSkip,
        const BSONObj *fieldsToReturn,
        int queryOptions, int batchSize)
{
    if (options_->slave_ok)
        queryOptions |= QueryOption_SlaveOk;
    return unique_ptr<DBClientCursor>(
               (*db_)->query(this->collection(coll), query, nToReturn, nToSkip,
                             fieldsToReturn, queryOptions, batchSize).release());
}

unsigned long long mongo::DB::count(const std::string &coll,
                                    const mongo::BSONObj& query, int options )
{
    if (options_->slave_ok)
        options |= QueryOption_SlaveOk;
    return (*db_)->count(this->collection(coll), query, options);
}

bool mongo::DB::createCollection(const std::string &coll, long long size,
                                 bool capped, int max, mongo::BSONObj *info)
{
    return (*db_)->createCollection(this->collection(coll), size,
                                    capped, max, info);
}

bool mongo::DB::dropCollection( const std::string &coll )
{
    return (*db_)->dropCollection(this->collection(coll));
}

bool mongo::DB::dropDatabase()
{
    return (*db_)->dropDatabase(database());
}


mongo::DB::ConnectionOptions *mongo::DB::options_by_name(const std::string &name)
{
    ConnectionOptionsMap::const_iterator it =
        connection_options_map_.find(name);
    if (it != connection_options_map_.end())
        return it->second;
    else
        return 0;
}

/** Добавляет настройки подключения */
void mongo::DB::_addDatabase(const std::string &name,
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

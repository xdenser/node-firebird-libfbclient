C++ NodeJS module to work with Firebird SQL Server. Uses libfbclient.
Inspired by ibpp, firebird samples, node-mysql-libmysql, node-postgress and other node modules used as sample source. 

# Features

* Synchronous and Asynchronous methods for connection, query and fetch data;
* Support of Firebird Events (post_event statement);
* Covered with tests in nodeunit;
* blob field support;
* blob stream compatible to node stream class;
* prepared statements;

As for now in plans are:

* connection pool support;
* prepared statements pool;
* transaction parameters support;
* continous refactoring;
* more tests;
* services api.

# Getting Started

You will need:
 NodeJS (tested with v0.4.4)
 Firebird (tested with v2.5)

 All things below were tested on CentOS 5.5.
 
Get Firebird:
 
    wget http://downloads.sourceforge.net/project/firebird/firebird-linux-i386/2.5-Release/FirebirdCS-2.5.0.26074-0.i686.rpm 
    rpm -ivh FirebirdCS-2.5.0.26074-0.i686.rpm 

Update your path: 
    export PATH=$PATH:/opt/firebird/bin

Create some Database: 
    isql -user sysdba -password masterkey
    CREATE DATABASE 'test.fdb';
    CONNECT 'test.fdb';
    CREATE TABLE TEST (id integer, name varchar(50));
    exit;

Clone repository and build module

    git clone git://github.com/xdenser/node-firebird-libfbclient.git
    node-waf configure build
    node-waf configure install

To run tests update ./tests/config.js with your test database connection parameters and

    git submodule update --init
    node-waf test
 
Play with it from node:

    var fb  = require("./firebird");
    sys = require("sys"); 
    var con = fb.createConnection();
    con.connectSync('test.fdb','sysdba','masterkey','');
    con.querySync("insert into test (id,name) values (5, 'new one')");
    var res = con.querySync("select * from test");
    con.commitSync();
    var rows = res.fetchSync("all",true);
    console.log(sys.inspect(rows));

Check also samples directory.    

# Links

[node.js and firebird installing on Amazon EC2 instance](http://mapopa.blogspot.com/2011/01/nodejs-and-firebird-installing-on.html) on Mariuz's Blog
[NodeJS home](http://nodejs.org)
[Collection of NodeJS modules](https://github.com/joyent/node/wiki/modules)
 
# Reference

 createConnection() method will create Firebird Connection object for you
 
## Connection object

Handles database connection and queries. Supports Synchronous and Asynchronous operation.

### Connection object members
* * *
    function connectSync(database, username, password, role);

where 

* `database` - string, a database name in Firebird notation, i.e. `<hostname>:<path to database file | alias>`
* `username` - string, user name
* `pasword` - string,
* `role` - string;

Connects you to database, raises exception on error (try to catch it).
Returns undefined.

* * *
    function connect(database, username, password, role, callback);
    
where first four parameters same as in connectSync()

* `callback` - function(err), where err is error object in case of error.

Asynchronously connects you to Database.
Returns udefined.

* * *
    connected;
A boolean readonly property indicating if Connection object is connected to database

* * *
    function querySync(sql);
* `sql` - string, an SQL query to execute.

Executes SQL query.
Returns FBResult object in case of success. Raises error otherwise.

* * *
    function query(sql, callback);

* `sql` - string, an SQL query to execute;
* `callback` - function(err,res), err - is error object or null, res - FBResult object.

Asynchronously executes query.
Returns undefined. 

* * *
    function addFBevent(name);
* `name` - string, Firebird Event Name.

Registers connection to listen for firebird event `name`, called from PL\SQL (in stored procedures or triggers) with post_event '`name`'.
You may set callback for event with `connection.on('fbevent', function(name, count){ <your code>));`. 
Where name is event name, and count is number of times event were posted.
    
* * *
    function deleteFBevent(name);
* `name` - string, Firebird Event Name.

Unsubscribes connection from getting events for name.
     
* * *
    function commitSync();
    
Synchronously commits current transaction. 

Notes:
There is only one transaction associated with connection. 
Transacation is automatically started before any query if connection does not have active transaction (check `inTransaction` property).
Use of mutliple transactions with same connection may be added in future.
You also should note that DDL statements (altering database structure) are commited automatically.

* * *
    function commit(callback);
* `callback` - function(err), where err is error object in case of error.    

Asynchronous commit transaction.Read notes in `commitSync();`.

* * *
    function rollbackSync();
    
Synchronously rollbacks current transaction. Read notes in `commitSync();`.

* * *
    function rollback(callback);
* `callback` - function(err), where err is error object in case of error.    
    
Asynchronously rollbacks current transaction. Read notes in `commitSync();`.

* * *
    function prepareSync(sql);
* `sql` - string, an SQL query to prepare.

Synchronously prepares SQL statement and returns FBStatement object.
    
* * *
    inTransaction;

A boolean readonly property indicating if connection is in started transaction state.
    
* * *

    function newBlobSync();

Creates new FBblob object and opens it for write. After finishing write operation and closing blob
one may insert it in database passing as parameter to exec, execSync methods of FBStatement object.

* * *    
## FBResult object

Represents results of SQL query if any. You should use this object to fetch rows from database.
Each row may be represented as array of field values or as object with named fields.

### Data types
Here is Interbase to Node data type accordance:

Firebird		Node  
DATE	->	Date  
TIME	->	Date  
TIMESTAMP	->	Date  
CHAR	->	String  
VARCHAR	->	String  
SMALLINT	->	Integer  
INTEGER	->	Integer  
NUMERIC	->	Integer, Number (depends on scale)  
DECIMAL	->	Integer, Number (depends on scale)  
FLOAT	->	Number  
DOUBLE	->	Number  
BLOB	->	FBblob  
  
      
### FBResult object members

* * *
    function fetchSync(rowCount, asObject);
    
* `rowCount` - integer|"all", number of rows to fetch from results;
* `asObject` - true|false, format of returned rows. When false - methods returns array of array, when true - array of objects.

Synchronously fetches result rows. If you pass "all" as rowCount - it will fetch all result rows. 
If you pass less rowCount than are actually in result, it will return specified number of rows. 
You may call fetchSync multiple times until all rows will be fetched. 
If you specify more rowCount than available it will return only actual number of rows.

* * *
    function fetch(rowCount, asObject, rowCallback, eofCallback);
    
* `rowCount` - integer|"all", number of rows to fetch from results;
* `asObject` - true|false, format of returned rows. When false - methods returns array of array, when true - array of objects;
* `rowCallback` - function(row), row - Array or Object (depends on asObject parameter) representing single row from result;
* `eofCallback` - function(err,eof), err - Error object in case of error, or null; eof - true | false.

Asynchronously fetches rows one by one. 
rowCallback is called for each fetched row. 
eofCallback is called when whole operation is complete. eof indicates if end of result set was met.

* * *
## FBStatement object

Represents prepared SQL query (returned by `Connection.prepare()` and `Connection.prepareSync()`). 
FBStatement is derived form FBResult class. So it can fetch rows just like FBresult object after call to execSync, exec methods.
    
### FBStatement object members

* * *
    function execSync(param1, param2, ..., paramN);
* `param1, param2, ..., paramN` - parameters of prepared statement in the same order as in SQL and with appropriate types.

Synchronously executes prepared statement with given parameters. You may fetch rows with methods inherited from FBResult.

* * *
    function exec(param1, param2, ..., paramN);
* `param1, param2, ..., paramN` - parameters of prepared statement in the same order as in SQL and with appropriate types.

Asynchronously executes prepared statement with given parameters. FBStatement emits 'result' or 'error' event. 
You may fetch rows with methods inherited from FBResult after 'result' event emitted.

* * *
## FBblob object

Represents BLOB data type.

### FBblob object members

* * *
    function _openSync();
    
Synchronously opens blob for reading.

* * *
    function _closeSync();
    
Synchronously closes previously opened blob.

* * *
    function _readSync(buffer);

* `buffer` - Node buffer to fill with data.

Synchronously reads BLOB segment (chunk) into buffer. Tries to fill whole buffer with data. 
Returns actual number of bytes read. 

* * *
    function _read(buffer, callback);

* `buffer` - Node buffer to fill with data.
* `callback` - function(err,buffer,len), err - Error object in case of error, or null;buffer - buffer filled with data; len - actual data length.

Asynchronously reads BLOB segment (chunk) into buffer. Tries to fill whole buffer with data. 

* * *
    function _readAll([[initialSize], [[chunkSize], [callback]]]);

* `initialSize` - optional, initial result buffer to allocate, default = 0;
* `chunkSize` - optional, size of chunk used to read data, default = 1024;
* `callback` - optional, function (err, buffer, len), err - Error object in case of error, or null;buffer - buffer filled with data; len - actual data length. 

Asynchronously reads all data from BLOB field. Object emits events while reading data `error`, `drain', `end`.

* * *
    function _writeSync(buffer,[len]);

* `buffer` - Node buffer to write from to blob;
* `len` - optional length parameter, if specified only len bytes from buffer will be writen.

Synchronously writes BLOB segment (chunk) from buffer.
Returns number of bytes actually writen.

* * *
    function _write(buffer,[len],[callback]);

* `buffer` - Node buffer to write from to blob;
* `len` - optional length parameter, if specified only len bytes from buffer will be writen.
* `callback` - function(err), err - Error object in case of error, or null;

Asynchronously writes BLOB segment (chunk) from buffer and calls callback function if any.

* * *
## Stream object

Represents BLOB stream. Create BLOB stream using `var strm = new fb.Stream(FBblob);`. 
You may pipe strm to/from NodeJS Stream objects (fs or socket). 
You may also look at [NodeJS Streams reference](http://nodejs.org/docs/v0.4.4/api/streams.html).




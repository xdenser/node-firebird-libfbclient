C++ NodeJS module to work with Firebird SQL Server. Uses fbclient library and with a few tweaks it can use libfbembedded

![Firebird Logo](https://www.totaljs.com/exports/firebird-logo.png)

[![NPM version][npm-version-image]][npm-url] [![NPM downloads][npm-downloads-image]][npm-url] [![Mozilla License][license-image]][license-url]
[![Build Status](https://travis-ci.org/xdenser/node-firebird-libfbclient.svg?branch=master)](https://travis-ci.org/xdenser/node-firebird-libfbclient)
[![Build status](https://ci.appveyor.com/api/projects/status/5do2fyjn6vx5yeci/branch/master?svg=true)](https://ci.appveyor.com/project/xdenser/node-firebird-libfbclient/branch/master)

[![NPM](https://nodei.co/npm/firebird.png?downloads=true&downloadRank=true)](https://nodei.co/npm/firebird/) [![NPM](https://nodei.co/npm-dl/firebird.png?months=6&height=3)](https://nodei.co/npm/firebird/)


# Features

* Synchronous and Asynchronous methods for connection, query and fetch data
* Support of Firebird Events (post_event statement)
* Covered with tests in nodeunit
* blob field support
* blob stream compatible to node stream class
* prepared statements

As for now in plans are:

* connection pool support
* prepared statements pool
* continous refactoring
* more tests
* services api

# Getting Started


##Under Linux, Windows and MacOS X
You will need:
 NodeJS (tested with v4.x or more)
 Firebird (tested with v2.5.x and v3.x)
 
 On Windows you need the [same requirements](https://github.com/atom/atom/blob/master/docs/build-instructions/windows.md) like Atom editor for building native C++ extensions 

Get Firebird.

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
    npm install

Alternate way is to install directly from github

    npm install xdenser/node-firebird-libfbclient
 
or use the version from npmjs:

    npm install firebird

To run tests update ./tests/config.js with your test database connection parameters and

    npm install nodeunit
    ./node_modules/.bin/nodeunit tests/def
     
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

Check also samples directory and [this sample application](https://github.com/xdenser/node-fb-sample).    


# Links

- [node.js and firebird installing on Amazon EC2 instance](http://mapopa.blogspot.com/2011/01/nodejs-and-firebird-installing-on.html) on Mariuz's Blog
- [Catch Firebird events with Node.js](http://www.king-foo.be/2011/07/catch-firebird-events-with-node-js) on www.king-foo.be 
- [NodeJS home](http://nodejs.org)
- [Collection of NodeJS modules](https://npmjs.org/)
 
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
You also should note that DDL statements (altering database structure) are commited automatically.
To run quieries in context of other transaction use Transaction object.

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
    function startSync();
    
Synchronously starts new default transaction. The default transaction should be not in started state before call to this method. Read notes in `commitSync();`.

* * *
    function start(callback);
* `callback` - function(err), where err is error object in case of error.    
    
Asynchronously starts new default transaction.. Read notes in `commitSync();`.

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

    function startNewTransactionSync();

Creates new Transaction object and starts new transaction. Returns created object.

* * *    

    function startNewTransaction(callback);
 * `callback` - function(err, transaction), where err is error object in case of error, transaction - newly created transaction.

Creates new Transaction object and starts new transaction. Returns created transaction object in callback.

* * *    

## Transaction object 

Represents SQL transaction. To get instance of this object call `startNewTransactionSync` or `startNewTransaction` methods of Connection object. Transaction objects may be reused after commit or rollback.

* * *
    function querySync(sql);
* `sql` - string, an SQL query to execute.

Executes SQL query in context of this transaction.
Returns FBResult object in case of success. Raises error otherwise.

* * *
    function query(sql, callback);

* `sql` - string, an SQL query to execute;
* `callback` - function(err,res), err - is error object or null, res - FBResult object.

Asynchronously executes query in context of this transaction.
Returns undefined. 

* * *
    function commitSync();
    
Synchronously commits this transaction. 

Notes:
Transacation is automatically started before any  query in context of this object if this object does not have active transaction (check `inTransaction` property).
You also should note that DDL statements (altering database structure) are commited automatically.

* * *
    function commit(callback);
* `callback` - function(err), where err is error object in case of error.    

Asynchronous commit transaction.Read notes in `commitSync();`.

* * *
    function rollbackSync();
    
Synchronously rollbacks transaction. Read notes in `commitSync();`.

* * *
    function rollback(callback);
* `callback` - function(err), where err is error object in case of error.    
    
Asynchronously rollbacks transaction. Read notes in `commitSync();`.

* * *
    function startSync();
    
Synchronously starts transaction. The transaction should be not in started state before call to this method. Read notes in `commitSync();`. See `inTransaction` property.

* * *
    function start(callback);
* `callback` - function(err), where err is error object in case of error.    
    
Asynchronously starts new transaction. Read notes in `commitSync();`.

* * *
    function prepareSync(sql);
* `sql` - string, an SQL query to prepare.

Synchronously prepares SQL statement and returns FBStatement object in context of this transaction.
    
* * *
    inTransaction;

A boolean readonly property indicating if this transaction is in started state.
    
* * *
## FBResult object

Represents results of SQL query if any. You should use this object to fetch rows from database.
Each row may be represented as array of field values or as object with named fields.

### Data types
Here is Firebird to Node data type accordance:

Firebird		Node  
DATE	->	Date  
TIME	->	Date  
TIMESTAMP	->	Date  
CHAR	->	String  
VARCHAR	->	String  
SMALLINT	->	Integer  
INTEGER	->	Integer  
NUMERIC	->	Number  
DECIMAL	->	Number  
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
Statement is executed in context of default connection transaction.

* * *
    function execInTransSync(transaction, param1, param2, ..., paramN);

Same as `execSync` but executes statement in context of given Transaction obejct.

* * *
    function exec(param1, param2, ..., paramN);
* `param1, param2, ..., paramN` - parameters of prepared statement in the same order as in SQL and with appropriate types.

Asynchronously executes prepared statement with given parameters. FBStatement emits 'result' or 'error' event. 
You may fetch rows with methods inherited from FBResult after 'result' event emitted.
Statement is executed in context of default connection transaction.

* * *
    function execInTrans(transaction, param1, param2, ..., paramN);

Same as `exec` but executes statement in context of given Transaction obejct.

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
You may also look at [NodeJS Streams reference](http://nodejs.org/api/stream.html).


[license-image]: http://img.shields.io/badge/license-MIT-blue.svg?style=flat
[license-url]: LICENSE

[npm-url]: https://npmjs.org/package/firebird
[npm-version-image]: http://img.shields.io/npm/v/firebird.svg?style=flat
[npm-downloads-image]: http://img.shields.io/npm/dm/firebird.svg?style=flat




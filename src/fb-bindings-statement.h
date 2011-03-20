/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#ifndef SRC_FB_BINDINGS_CONNECTION_H_
#define SRC_FB_BINDINGS_CONNECTION_H_

#include "./fb-bindings.h"
#include "./fb-bindings-connection.h"
#include "./fb-bindings-fbeventemitter.h"

class FBStatement : public FBEventEmitter {



protected:
 FBStatement(Connection* aconn); 
 ~FBStatement(); 

private: 
 Connection* conn;

}


#endif

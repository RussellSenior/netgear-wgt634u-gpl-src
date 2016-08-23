///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// * Neither name of Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

//	$Revision: 1.1.1.3 $
//	$Date: 2001/06/15 00:22:14 $

/*****************************************************************/
//	Class : Node
//	File  : Node.cpp
//	Description:  This class implements the DOM Node Object
//	Refer to DOM Level1 Spec for comments about all the interfaces.
/*****************************************************************/

#ifndef _NODE_H_
#define _NODE_H_

#include <iostream.h>
#include <fstream.h>
#include "Parser.h"
#include "NodeAct.h"
#include "NodeList.h"
#include "NamedNodeMap.h"
#include "all.h"
#include "DOMException.h"

class NodeAct;
class Parser;
class NodeList;
class NamedNodeMap;
class Document;

class Node
{
public:

	Node();//Constructor
	~Node();//Destructor: decerements ref count may not actually remove the nodeact
	//DOM Level 1 functions
	char*			getNodeName();
	char*			getNodeValue();
	void			setNodeValue(char *newNodeValue);
	unsigned short	getNodeType();
	Node&			getParentNode();
	NodeList&		getChildNodes();
	Node&			getFirstChild();
	Node&			getLastChild();
	Node&			getPreviousSibling();
	Node&			getNextSibling();
	NamedNodeMap&	getAttributes();
	Document&		getOwnerDocument();

	Node&			insertBefore(Node& newChild, Node& refChild);
	Node&			replaceChild(Node& newChild, Node& oldChild);
	Node&			removeChild(Node& oldChild);
	Node&			appendChild(Node& newChild);
	bool			hasChildNodes();
	Node&			cloneNode(bool deep);

	//Necessary functions
	Node& operator = (const Node &other);
	bool isNull();
	void deleteNode();

	//Other Internal functions
	//CreateNode Creates a Node. Not part of DOM, but a very useful function
	void createNode(Node **returnNode, NODE_TYPE nt,char *NodeName, char *NodeValue);
	
	//member variables
	NodeAct *nact;
	Node *ownerNode;

private:

protected:
    void SearchList(Node& n, char *tagname, NodeList **lst, bool ignorePrefix);
};

#endif  // Node.h


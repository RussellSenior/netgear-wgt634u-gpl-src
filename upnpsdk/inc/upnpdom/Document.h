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

#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include "Node.h"
#include "NodeAct.h"
#include "NodeList.h"
#include "all.h"
#include "Attr.h"
#include "Element.h"
#include "DOMException.h"

class Node;
class Element;
class Attr;
class Parser;
class NodeList;

#define MAX_FILE_SIZE 10000

class Document : public Node
{
public:
	Document();
	~Document();

	//DOMLevel1 Interfaces
	Document& createDocument();
	Attr& createAttribute(char *name);
	Element& createElement(char *tagName);
	NodeList& getElementsByTagName( char * tagName);
	Node& createTextNode(char *data);

	//Additional Interfaces
    	Document& operator = (const Document &other);
	Document& ReadDocumentFileOrBuffer(char * xmlFile, bool file);
	Attr& createAttribute(char *name, char *value);
	void  createDocument(Document **returnDoc); 
	void deleteDocument();
	void deleteDocumentTree();
	Document *ownerDocument;

private:
	NodeAct *CurrentNodePtr;
	//void MakeNodeList(Node& n, char* match, NodeList **nl);
    void SearchForTag(Node& n, char *tagname, NodeList **lst, bool ignorePrefix);
protected:
};

#endif



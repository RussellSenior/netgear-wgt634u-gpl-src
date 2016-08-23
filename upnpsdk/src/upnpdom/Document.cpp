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

//	$Revision: 1.1.1.5 $
//	$Date: 2001/06/15 00:22:16 $
#include "../../inc/tools/config.h"
#if EXCLUDE_DOM == 0
#include "../../inc/upnpdom/Document.h"
#include "../../inc/upnpdom/domCif.h"
#include <stdio.h>
#include <stdlib.h>

Document::Document()
{
	ownerDocument=this;
	this->nact=NULL;
}

Document::~Document()
{
}

void 	Document::createDocument(Document **returnDoc)
{
	*returnDoc = new Document;
	if(!(*returnDoc))
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	(*returnDoc)->nact= new NodeAct(DOCUMENT_NODE, "#document",NULL, (Node*)returnDoc);
	(*returnDoc)->nact->OwnerNode=(*returnDoc)->nact;
	(*returnDoc)->nact->RefCount++;
}

Document& 	Document::createDocument()
{
	Document *returnDoc;
	returnDoc = new Document;
	if(!returnDoc)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnDoc->nact= new NodeAct(DOCUMENT_NODE, "#document",NULL, (Node*)returnDoc);
	if(!(returnDoc->nact))
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnDoc->nact->OwnerNode=returnDoc->nact;
	returnDoc->nact->RefCount++;
	return *returnDoc;
}
	
	
Attr&	Document::createAttribute(char *name)
{
	Attr *returnAttr;
	returnAttr = new Attr(name);
	if(!returnAttr)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	return *returnAttr;
}

Attr& 	Document::createAttribute(char *name, char *value)
{
	Attr *returnAttr;
	returnAttr = new Attr(name, value);
	if(!returnAttr)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	return *returnAttr;
}

Element& Document::createElement(char *tagName)
{
	Element *returnElement;
	returnElement = new Element(tagName);
	if(!returnElement)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	return *returnElement;
}

Node& 	Document::createTextNode(char *data)
{
	Node *returnNode;
	returnNode->createNode(&returnNode,TEXT_NODE,"#text",data);
	return *returnNode;
}
	
	
Document& Document::ReadDocumentFileOrBuffer(char * xmlFile, bool file)
{
	//to do: try handling this in a better fashin than allocating a static length..
	char *fileBuffer;
	FILE *XML;
	Document *RootDoc;
	NODE_TYPE NodeType;
	char *NodeName=NULL;
	char *NodeValue=NULL;
	bool IsEnd;
	bool IgnoreWhiteSpace=true;
	Node *a;
	Node *b;
	Element *back;

	DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"Inside ReadDocumentFileOrBuffer function\n");)
	if(file)
	{
        	XML=fopen(xmlFile, "r");
        	if(!XML)
        	{
       			DBGONLY(UpnpPrintf(UPNP_INFO,DOM,__FILE__,__LINE__,"%s - No Such File Or Directory \n", xmlFile);)
        		throw DOMException(DOMException::NO_SUCH_FILE, NULL);
        	}
        	else{
	       			fseek(XML,0, SEEK_END);
	       			int fz = ftell(XML);
	       			fileBuffer = (char *)malloc(fz+2);
	       			if(fileBuffer == NULL)
	       			{
       					DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
	       				throw DOMException(DOMException::INSUFFICIENT_MEMORY, NULL);
	       			}
	       			fseek(XML,0, SEEK_SET);
	       			int sizetBytesRead = fread (fileBuffer, 1, fz, XML);
        			fileBuffer[sizetBytesRead] = '\0'; // append null
        			fclose (XML);
        	}
 	}
 	else
 	{
 		fileBuffer=(char *)malloc(strlen(xmlFile)+1);
		if(fileBuffer == NULL)
		{
			DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
			throw DOMException(DOMException::INSUFFICIENT_MEMORY, NULL);
		}
		strcpy(fileBuffer, xmlFile);
 	}
    try
    { 		   	
	Parser myparser(fileBuffer);
	free(fileBuffer);
   	createDocument(&RootDoc);
   	RootDoc->CurrentNodePtr=RootDoc->nact;
   	back=NULL;
	while(1)
	{
		Element ele;
		Attr attr;
		try
		{
    		if(myparser.getNextNode(NodeType, &NodeName, &NodeValue, IsEnd, IgnoreWhiteSpace)==0)
    		{
				DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"NextNode while parsing Nodetype %d, Nodename %s, Nodevalue %s\n", NodeType, NodeName, NodeValue);)
        		if(!IsEnd)
        		{
        			switch(NodeType)
        			{
        			case ELEMENT_NODE:	
        								ele=createElement(NodeName);
        								a=(Node*)&ele;
        								RootDoc->CurrentNodePtr->appendChild(a->nact);
        								RootDoc->CurrentNodePtr=RootDoc->CurrentNodePtr->LastChild;
        								delete ele.ownerElement;
        								break;
        			case TEXT_NODE:		
        								Node::createNode(&b,NodeType,NodeName,NodeValue);
        								RootDoc->CurrentNodePtr->appendChild(b->nact);
        								delete b;
        								break;
        			case ATTRIBUTE_NODE:
        								attr=createAttribute(NodeName, NodeValue);
        								a=(Node*)&attr;
        								RootDoc->CurrentNodePtr->appendChild(a->nact);
        								delete attr.ownerAttr;
        								break;
        			case INVALID_NODE:
        								break;
        			default:   break;
        			}
        		}
        		//Throw exception if it is an invalid document
        		else
        		{
        			if(!strcmp(NodeName, RootDoc->CurrentNodePtr->NA_NodeName))
        			{
        				RootDoc->CurrentNodePtr = RootDoc->CurrentNodePtr->ParentNode;
        			}
        			else
        			{
        				RootDoc->deleteDocumentTree();
						DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"fatal error during parsing\n");)       				
        				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
        			}
        		}
        		if(NodeType==INVALID_NODE)
        			break;
        		else
        			NodeType=INVALID_NODE;
        		if(NodeName)
        		{										
        			Upnpfree(NodeName);
        			NodeName=NULL;
        		}
        		if(NodeValue)
        		{
        			Upnpfree(NodeValue);
        			NodeValue=NULL;
        		}
        		IsEnd=false;
      		}
      		else
      		{
    			RootDoc->deleteDocumentTree();
    			throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
    		}
  		}
  		catch(DOMException& toCatch)
  		{
   			RootDoc->deleteDocumentTree();
			delete RootDoc;
   			throw DOMException(toCatch.code);
   		}
 	}
   	}catch(DOMException& toCatch)
   	{
   	    RootDoc=NULL;
	    throw DOMException(toCatch.code);
	}
	RootDoc->ownerDocument=RootDoc;
	DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"**EndParse**\n");)
	return *RootDoc;
}

void 	Document::deleteDocumentTree()
{
	if(ownerDocument->nact == NULL)
		return;
	ownerDocument->nact->deleteNodeAct();
	delete ownerDocument->nact;
	ownerDocument->nact=NULL;
}

void 	Document::deleteDocument()
{
	delete ownerDocument;
	this->nact=NULL;
}

Document& Document::operator = (const Document &other)
{
    if (this->nact != other.nact)
    {
        this->nact = other.nact;
		this->nact->RefCount++;
    }
	this->ownerDocument=other.ownerDocument;
    return *this;
};

NodeList& Document::getElementsByTagName( char * tagName)
{
	NodeList *returnNodeList;
	returnNodeList= new NodeList;
	if(!returnNodeList)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnNodeList->ownerNodeList=returnNodeList;
	DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"Calling makeNodeList\n");)
    SearchList(*this, tagName, &returnNodeList, strchr(tagName,':')==NULL);
	return *returnNodeList;
}


#endif

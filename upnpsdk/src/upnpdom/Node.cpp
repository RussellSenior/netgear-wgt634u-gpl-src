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
#include "../../inc/upnpdom/Node.h"
#include "../../inc/upnpdom/Document.h"

Node::Node()
{
	this->nact=NULL;
	ownerNode=this;
}

//The destructor function checks to see if no other node is referencing its node act
//Only then it destroys its nodeact.
Node::~Node()
{
	if(nact !=NULL)
	{
		nact->RefCount--;
		if(nact->RefCount==0)//check for children and delete recursively..
		{
			nact->deleteNodeAct();
			delete nact;
		}
	}
	nact=NULL;
}

//Returns the nodename of this node
//User has to free the memory
char*	Node::getNodeName()
{
	if(nact !=NULL)
	{
		char *retNodeName=NULL;
		if(!nact->NA_NodeName)
			return NULL;
		retNodeName = new char[strlen(nact->NA_NodeName)+1];
		if(!retNodeName)
		{
			DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
			throw DOMException(DOMException::INSUFFICIENT_MEMORY);
		}
		strcpy(retNodeName, nact->NA_NodeName);
		return retNodeName;
	}
	else
		throw DOMException(DOMException::NO_SUCH_NODE);
}

//Returns the nodeValue of this node
//User has to free the memory
char*	Node::getNodeValue()
{
	if(nact !=NULL)
	{
		char *retNodeValue=NULL;
		if(!nact->NA_NodeValue)
			return NULL;
		retNodeValue = new char[strlen(nact->NA_NodeValue)+1];
		if(!retNodeValue)
		{
			DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
			throw DOMException(DOMException::INSUFFICIENT_MEMORY);
		}
		strcpy(retNodeValue, nact->NA_NodeValue);
		return retNodeValue;
	}
	else
		throw DOMException(DOMException::NO_SUCH_NODE);
}

//Sets the nodeValue of this node if the node is present
//If the node is not found throws an exception
void	Node::setNodeValue(char *newNodeValue)
{
	if(nact!=NULL)
	{
		if(this->nact->NA_NodeValue !=NULL)
			delete this->nact->NA_NodeValue;
		this->nact->NA_NodeValue =NULL;	
		this->nact->NA_NodeValue=new char[strlen(newNodeValue)+1];
		if(!this->nact->NA_NodeValue)
		{
			DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
			throw DOMException(DOMException::INSUFFICIENT_MEMORY);
		}
		strcpy(this->nact->NA_NodeValue, newNodeValue);
	}
	else
		throw DOMException(DOMException::NO_SUCH_NODE);
}

//Gets the NodeType of this node
unsigned short Node::getNodeType()
{
	if(nact!=NULL)
		return(this->nact->NA_NodeType);
	else
	{
		throw DOMException(DOMException::NO_SUCH_NODE);
		return 0;
	}
}

//Returns the parent node of the node under reference
//To see if it actually returned NULL use isNull() function
Node&	Node::getParentNode()
{
	Node *returnNode;
	returnNode = new Node();
	if(!returnNode)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnNode->nact=this->nact->ParentNode;
	if(returnNode->nact!=NULL)
		returnNode->nact->RefCount++;
	return *returnNode;
}

//Returns the first child of the node under reference
//To see if it actually returned NULL use isNull() function
Node&	Node::getFirstChild()
{
	Node *returnNode;
	returnNode = new Node();
	if(!returnNode)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnNode->nact=this->nact->FirstChild;
	if(returnNode->isNull())
		return *returnNode;
	returnNode->nact->RefCount++;
	return *returnNode;
}

//Returns the last child of the node under reference
//To see if it actually returned NULL use isNull() function
Node&	Node::getLastChild()
{
	Node *returnNode;
	returnNode = new Node();
	if(!returnNode)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnNode->nact=this->nact->LastChild;
	if(returnNode->isNull())
		return *returnNode;
	returnNode->nact->RefCount++;
	return *returnNode;
}

//Returns the previous sibling of the node under reference
//To see if it actually returned NULL use isNull() function
Node&	Node::getPreviousSibling()
{
	Node *returnNode;
	returnNode = new Node();
	if(!returnNode)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnNode->nact=this->nact->PrevSibling;
	if(returnNode->isNull())
		return *returnNode;
	returnNode->nact->RefCount++;
	return *returnNode;
}

//Returns the next sibling of the node under reference
//To see if it actually returned NULL use isNull() function
Node&	Node::getNextSibling()
{
	Node *returnNode;
	returnNode = new Node();
	if(!returnNode)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnNode->nact=this->nact->NextSibling;
	if(returnNode->isNull())
		return *returnNode;
	returnNode->nact->RefCount++;
	return *returnNode;
}

//Gets the owner Document
//To see if it actually returned NULL use isNull() function
Document&	Node::getOwnerDocument()
{
	Document *returnDoc;
	returnDoc = new Document;
	if(!returnDoc)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnDoc->nact= this->nact->OwnerNode;
	returnDoc->nact->RefCount++;
	return *returnDoc;
}

void Node::createNode(Node **returnNode, NODE_TYPE nt,char *NodeName, char *NodeValue)
{
	*returnNode = new Node;
	if(!returnNode)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	(*returnNode)->nact= new NodeAct(nt, NodeName,NodeValue, *returnNode);
	if(!((*returnNode)->nact))
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}	
	(*returnNode)->nact->RefCount++;
}

Node&	Node::insertBefore(Node& newChild, Node& refChild)
{
	Node *returnNode;
	returnNode = new Node();
	if(!returnNode)
	{
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
		throw DOMException(DOMException::INSUFFICIENT_MEMORY);
	}
	try
	{
		nact->insertBefore(newChild.nact, refChild.nact);
	}
	catch (DOMException& toCatch)
	{
		throw DOMException( toCatch.code);
	}
	returnNode->nact=newChild.nact;
	return *returnNode;
}

Node&	Node::replaceChild(Node& newChild, Node& oldChild)
{
	Node *returnNode;
	returnNode = new Node();
	if(!returnNode)
	{
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
		throw DOMException(DOMException::INSUFFICIENT_MEMORY);
	}
	try
	{
		nact->replaceChild(newChild.nact, oldChild.nact);
	}
	catch (DOMException& toCatch)
	{
		throw DOMException( toCatch.code);
	}
	newChild.nact=oldChild.nact;
	returnNode->nact=newChild.nact;
	return *returnNode;
}

Node&	Node::removeChild(Node& oldChild)
{
	Node *returnNode;
	returnNode = new Node();
	if(!returnNode)
	{
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
		throw DOMException(DOMException::INSUFFICIENT_MEMORY);
	}
	try
	{
		nact->removeChild(oldChild.nact);
	}
	catch (DOMException& toCatch)
	{
		throw DOMException( toCatch.code);
	}
	returnNode->nact=oldChild.nact;
	return *returnNode;
}
	
Node& Node::appendChild(Node& newChild)
{
	Node *returnNode;
	returnNode = new Node();
	if(!returnNode)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	nact->appendChild(newChild.nact);
	returnNode->nact=newChild.nact;
	return *returnNode;
}

Node& Node::cloneNode(bool deep)
{
	Node *returnNode;
	returnNode = new Node();
	if(!returnNode)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnNode->nact=this->nact->cloneNode(deep);
	return *returnNode;
}

NodeList& Node::getChildNodes()
{
	NodeAct *na;
	NodeList *returnNodeList;
	returnNodeList= new NodeList;
	if(!returnNodeList)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnNodeList->ownerNodeList=returnNodeList;
	na=nact->FirstChild;
	while(na !=NULL)
	{
		returnNodeList->addToInternalList(na);
		na=na->NextSibling;
	}
	return *returnNodeList;
}


NamedNodeMap& Node::getAttributes()
{
	NamedNodeMap *returnNamedNodeMap;
	returnNamedNodeMap= new NamedNodeMap;
	if(!returnNamedNodeMap)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnNamedNodeMap->ownerNamedNodeMap=returnNamedNodeMap;
	returnNamedNodeMap->ofWho=this;
	return *returnNamedNodeMap;
}

bool Node::hasChildNodes()
{
	return(nact->FirstChild!=NULL);
}

bool Node::isNull()
{
    return nact == NULL;
}

//Call this function whenever a node is returned back to you
//Also one can use this function in place of delete when created on the heap.
void	Node::deleteNode()
{
	delete this->ownerNode;
	this->nact=NULL;
}


Node & Node::operator = (const Node &other)
{
    if (this->nact != other.nact)
    {
        this->nact = other.nact;
		if(this->nact !=NULL)
			this->nact->RefCount++;
    }
	this->ownerNode=other.ownerNode;
    return *this;
};

void Node::SearchList(Node& n, char *tagname, NodeList **lst, bool ignorePrefix) {
    Node child = n.getFirstChild();
    Node temp;
    while (!child.isNull()) {
        temp=child.getNextSibling();
        char *name;
        char *name2;
        if (child.getNodeType()==ELEMENT_NODE){
            name=child.getNodeName();
            if (ignorePrefix) {
                name2=strchr(name,':');
                if (name2==NULL) name2=name;
                else name2++;
            }
            else name2=name;
            if (strcmp(tagname,name2)==0||strcmp(tagname,"*")==0)
                (*lst)->addToInternalList(child.nact);
            delete name;
            }
        SearchList(child, tagname, lst, ignorePrefix);
        child = temp;
    }
    if(child.isNull())
    	child.deleteNode();
	if(n.getNodeType()!=DOCUMENT_NODE)
		n.deleteNode();
}


#endif


	




	


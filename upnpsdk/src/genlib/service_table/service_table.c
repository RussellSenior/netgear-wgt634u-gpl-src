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
//
// $Revision: 1.1.1.5 $
// $Date: 2001/06/15 00:21:55 $
//     
#include "../../inc/tools/config.h"
#if EXCLUDE_DOM == 0
#include "./genlib/service_table/service_table.h"


DEVICEONLY(
int copy_subscription(subscription *in, subscription *out)
{
  int return_code=HTTP_SUCCESS;

  memcpy(out->sid,in->sid,SID_SIZE);
  out->sid[SID_SIZE]=0;
  out->eventKey=in->eventKey;
  out->ToSendEventKey=in->ToSendEventKey;
  out->expireTime=in->expireTime;
  out->active=in->active;
  if ( (return_code=copy_URL_list(&in->DeliveryURLs,&out->DeliveryURLs))!=HTTP_SUCCESS)
    return return_code;
  out->next=NULL; 
  return HTTP_SUCCESS; 
}

void RemoveSubscriptionSID(Upnp_SID sid, service_info * service)
{
  subscription * finger=service->subscriptionList;
  subscription * previous=NULL;
  
  while (finger)
    {
      if ( !(strcmp(sid,finger->sid)))
	{
	  if (previous)
	    previous->next=finger->next;
	  else
	    service->subscriptionList=finger->next;
	  finger->next=NULL;
	  freeSubscriptionList(finger);
	  finger=NULL;
	  service->TotalSubscriptions--;
	}
      else
	{ previous=finger;
	  finger=finger->next;
	}
    }
  
}


subscription * GetSubscriptionSID(Upnp_SID sid,service_info * service)
{
  subscription * next =service->subscriptionList;
  subscription * previous =NULL;
  subscription * found=NULL;

  time_t current_time;

  while ( (next) && (found==NULL))
    {
      if ( ! strcmp(next->sid,sid))
	    found=next;
      else 
	{
	  previous=next;
	  next=next->next;
	}
    } 
  if (found)
    {
       //get the current_time
      time(&current_time);
      if ( (found->expireTime!=0) && (found->expireTime<current_time) )
	{
	  if (previous)
	    previous->next=found->next;
	  else 
	    service->subscriptionList=found->next;
	  found->next=NULL;
	  freeSubscriptionList(found);
	  found=NULL;
	  service->TotalSubscriptions--;
	}
    }
  return found;

}


subscription * GetNextSubscription(service_info * service, subscription *current)
{
  time_t current_time;
  subscription * next=NULL;
  subscription * previous=NULL;
  int notDone =1;

  //get the current_time
  time(&current_time);
  while ( (notDone) && (current))
    {
      previous=current;
      current=current->next;

      if (current==NULL)
	{
	  notDone=0;
	  next=current;
	}
      else
	if ( (current->expireTime!=0) && (current->expireTime<current_time) )
	  {
	    previous->next=current->next;
	    current->next=NULL;
	    freeSubscriptionList(current);
	    current=previous;
	    service->TotalSubscriptions--;
	  }
	    else
	  if (current->active)
	    {
	      notDone=0;
	      next=current;
	    }
    }
  return next;
}

subscription * GetFirstSubscription(service_info *service)
{
  subscription temp; 
  subscription * next=NULL;
  temp.next=service->subscriptionList;
  next=GetNextSubscription(service, &temp);
  service->subscriptionList=temp.next;
  //  service->subscriptionList=next;
  return next;
}





service_info * FindServiceId(service_table *table, const char * serviceId, const char * UDN)
{
  service_info * finger=NULL;

  if (table)
    {
      finger=table->serviceList;
      while(finger)
	{
	  if (  ( !strcmp(serviceId,finger->serviceId)) && 
		( !strcmp(UDN,finger->UDN)))
	    {
	      return finger;
	    }
	  finger=finger->next;
	}
    }

  return NULL;
}

service_info * FindServiceEventURLPath(service_table *table, 
				       char * eventURLPath)
{
  service_info * finger=NULL;
  uri_type parsed_url;
  uri_type parsed_url_in;

  if ( (table) && (parse_uri(eventURLPath,strlen(eventURLPath),&parsed_url_in)))
    {
     
      finger=table->serviceList;
      while (finger)
	{  
	  if (finger->eventURL)
	  if ( (parse_uri(finger->eventURL,strlen(finger->eventURL),
			  &parsed_url)))
	    {
	     
	      if (!token_cmp(&parsed_url.pathquery,&parsed_url_in.pathquery))
		return finger;
	      
	    }
	  finger=finger->next;
	}
    }
  
  return NULL;
}

service_info * FindServiceControlURLPath(service_table * table, char * controlURLPath)
{
  service_info * finger=NULL;
  uri_type parsed_url;
  uri_type parsed_url_in;

  if ( (table) && (parse_uri(controlURLPath,strlen(controlURLPath),&parsed_url_in)))
    {
      finger=table->serviceList;
      while (finger)
	{
	  if (finger->controlURL)
	  if ( (parse_uri(finger->controlURL,strlen(finger->controlURL),
			  &parsed_url)))
	    {
	      if (!token_cmp(&parsed_url.pathquery,&parsed_url_in.pathquery))
		return finger;
	    }
	  finger=finger->next;
	}
    }

  return NULL;
  
}

DBGONLY(
void printService(service_info *service, Dbg_Level level,
		  Dbg_Module module)
{
  if (service)
    {
      if (service->serviceType)
	UpnpPrintf(level,module,__FILE__,__LINE__,"serviceType: %s\n",service->serviceType);
      if (service->serviceId)
	UpnpPrintf(level,module,__FILE__,__LINE__,"serviceId: %s\n",service->serviceId);
      if (service->SCPDURL)
	UpnpPrintf(level,module,__FILE__,__LINE__,"SCPDURL: %s\n",service->SCPDURL);
      if (service->controlURL)
	UpnpPrintf(level,module,__FILE__,__LINE__,"controlURL: %s\n",service->controlURL);
      if (service->eventURL)
	UpnpPrintf(level,module,__FILE__,__LINE__,"eventURL: %s\n",service->eventURL);
      
      if (service->UDN)
	UpnpPrintf(level,module,__FILE__,__LINE__,"UDN: %s\n\n",service->UDN);

      if (service->active)
	UpnpPrintf(level,module,__FILE__,__LINE__,"Service is active\n");
      else
	UpnpPrintf(level,module,__FILE__,__LINE__,"Service is inactive\n");
    }
})

DBGONLY(
void printServiceList(service_info *service,Dbg_Level level,
		      Dbg_Module module)
{
  while (service)
    {
      if (service->serviceType)
	UpnpPrintf(level,module,__FILE__,__LINE__,"serviceType: %s\n",service->serviceType);
      if (service->serviceId)
	UpnpPrintf(level,module,__FILE__,__LINE__,"serviceId: %s\n",service->serviceId);
      if (service->SCPDURL)
	UpnpPrintf(level,module,__FILE__,__LINE__,"SCPDURL: %s\n",service->SCPDURL);
      if (service->controlURL)
	UpnpPrintf(level,module,__FILE__,__LINE__,"controlURL: %s\n",service->controlURL);
      if (service->eventURL)
	UpnpPrintf(level,module,__FILE__,__LINE__,"eventURL: %s\n",service->eventURL);
      
      if (service->UDN)
	UpnpPrintf(level,module,__FILE__,__LINE__,"UDN: %s\n\n",service->UDN);

      if (service->active)
	UpnpPrintf(level,module,__FILE__,__LINE__,"Service is active\n");
      else
	UpnpPrintf(level,module,__FILE__,__LINE__,"Service is inactive\n");

      service=service->next;
    }
})

DBGONLY(
void printServiceTable(service_table * table,Dbg_Level level,
  Dbg_Module module)
{
  UpnpPrintf(level,module,__FILE__,__LINE__,"URL_BASE: %s\n",table->URLBase);
  UpnpPrintf(level,module,__FILE__,__LINE__,"Services: \n");
  printServiceList(table->serviceList,level,module);
}
)

void freeSubscription(subscription * sub)
{
  if (sub)
    {
      free_URL_list(&sub->DeliveryURLs);
    }
}

void freeSubscriptionList(subscription * head)
{
  subscription * next=NULL;
  while (head)
    {
      next=head->next;
      freeSubscription(head);
      free(head);
      head=next;
    }
}

void freeServiceList(service_info * head)
{
  service_info *next=NULL;
  while (head)
    {
      if (head->serviceType)
	UpnpDOMString_free(head->serviceType);
      if (head->serviceId)
	UpnpDOMString_free(head->serviceId);
      if (head->SCPDURL)
	free(head->SCPDURL);
      if (head->controlURL)
	free(head->controlURL);
      if (head->eventURL)
	free(head->eventURL);
      if (head->UDN)
	UpnpDOMString_free(head->UDN);
      if (head->subscriptionList)
	freeSubscriptionList(head->subscriptionList);
      head->TotalSubscriptions=0;
      next=head->next;
      free(head);
      head=next;
    }
}



void freeServiceTable(service_table * table)
{
  UpnpDOMString_free(table->URLBase);
  freeServiceList(table->serviceList);
  table->serviceList=NULL;
}



Upnp_DOMString getElementValue(Upnp_Node node)
{
  Upnp_Node child = UpnpNode_getFirstChild(node);
  Upnp_DOMException err;
  Upnp_DOMString temp=NULL;

 
  if ( (child!=0) && (UpnpNode_getNodeType(child)==TEXT_NODE))
    {
      temp=  UpnpNode_getNodeValue(child,&err);
      UpnpNode_free(child);
     
      return temp;
    }
  else
    {
      
    return NULL;
    }
}

int getSubElement(const char * element_name,Upnp_Node node, 
			Upnp_Node *out)
{
  
  Upnp_DOMString NodeName=NULL;
  
  int found=0;
  Upnp_Node temp=NULL;
 
  Upnp_Node child=UpnpNode_getFirstChild(node);
  (*out)=NULL;
  while ( (child!=NULL) && (!found))
    {
      
      switch(UpnpNode_getNodeType(child))
	{
	case ELEMENT_NODE : 
	  
	  NodeName =UpnpNode_getNodeName(child);
	  if (!strcmp(NodeName,element_name))
	    { 
	      (*out)=child;
	      found=1;
	      UpnpDOMString_free(NodeName);
	      return found;
	    }
	  UpnpDOMString_free(NodeName);
	  break;
	default: ;	  
	}
      
      temp=UpnpNode_getNextSibling(child);
      UpnpNode_free(child);
      child=temp;
    }
  return found;
}

service_info * getServiceList(Upnp_Node node,service_info **end, char * URLBase)
{
  Upnp_Node serviceList=NULL;
  Upnp_Node current_service=NULL;
  

  Upnp_Node UDN=NULL;

  Upnp_Node serviceType=NULL;
  Upnp_Node serviceId=NULL;
  Upnp_Node SCPDURL=NULL;
  Upnp_Node controlURL=NULL;
  Upnp_Node eventURL=NULL;

  service_info * head=NULL;
  service_info * current=NULL;
  service_info * previous=NULL;
  Upnp_DOMString tempDOMString=NULL;
  Upnp_NodeList serviceNodeList=NULL;
  int NumOfServices=0;
  int i=0;
  int fail=0;
  

  
  if ( (getSubElement("UDN",node,&UDN)) && (getSubElement("serviceList",node,&serviceList)) )
    {
      serviceNodeList=UpnpElement_getElementsByTagName(serviceList, "service");
      
      if (serviceNodeList!=NULL)
	{
	  NumOfServices=UpnpNodeList_getLength(serviceNodeList);
	  for (i=0;i<NumOfServices;i++)
	    {
	      current_service=UpnpNodeList_item(serviceNodeList,i);
	      
	      fail=0;

	      if (current)
		{
		  current->next=(service_info* ) malloc(sizeof(service_info));
		  previous=current;
		  current=current->next; 
		}
	      else
		{
		  head = (service_info * ) malloc (sizeof(service_info));
		  current=head;
		}
	      
	      if (!current)
		{
		  freeServiceList(head);
		  return NULL;
		}
	      
	      current->next=NULL;
	      current->controlURL=NULL;
	      current->eventURL=NULL;
	      current->serviceType=NULL;
	      current->serviceId=NULL;
	      current->SCPDURL=NULL;
	      current->active=1;
	      current->subscriptionList=NULL;
	      current->TotalSubscriptions=0;

	      if (!(current->UDN=getElementValue(UDN)))
		fail=1;

	      

	      if  ( (!getSubElement("serviceType",current_service,&serviceType)) ||
		    (!(current->serviceType=getElementValue(serviceType)) ) )
		fail=1;
	      
	      
	      if  ( (!getSubElement("serviceId",current_service,&serviceId)) ||
		    (!(current->serviceId=getElementValue(serviceId))))
		fail=1;
	      
	      
	      if ( (!(getSubElement("SCPDURL",current_service,&SCPDURL))) ||
	      	   (!(tempDOMString=getElementValue(SCPDURL))) || 
	         (!(current->SCPDURL=resolve_rel_url(URLBase,tempDOMString))))
		fail=1;
	      
	      UpnpDOMString_free(tempDOMString);
	      tempDOMString=NULL;
	      
	      if ( (!(getSubElement("controlURL",current_service,&controlURL))) ||
		   (!(tempDOMString=getElementValue(controlURL))) || 
		   (!(current->controlURL=resolve_rel_url(URLBase,tempDOMString))))
		{
		  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"BAD OR MISSING CONTROL URL"));
		  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"CONTROL URL SET TO NULL IN SERVICE INFO"));
		  current->controlURL=NULL;
		  fail=0;
		}

	      UpnpDOMString_free(tempDOMString);
	      tempDOMString=NULL;

	      if ( (!(getSubElement("eventSubURL",current_service,&eventURL))) ||
		   (!(tempDOMString=getElementValue(eventURL))) || 
		   (!(current->eventURL=resolve_rel_url(URLBase,tempDOMString))))
		{
		  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"BAD OR MISSING EVENT URL"));
		  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"EVENT URL SET TO NULL IN SERVICE INFO"));
		  current->eventURL=NULL;
		  fail=0;
		}
	      UpnpDOMString_free(tempDOMString);
	      tempDOMString=NULL;
	      
	      if (fail)
		{
		  freeServiceList(current);
		  if (previous)
		    previous->next=NULL;
		  else
		    head=NULL;
		  current=previous;
		}
	      UpnpNode_free(controlURL);
	      UpnpNode_free(eventURL);
	      UpnpNode_free(SCPDURL);
	      UpnpNode_free(serviceId);
	      UpnpNode_free(serviceType);
	      UpnpNode_free(current_service);
	    }
	  
	  UpnpNodeList_free(serviceNodeList);
	}
      (*end)=current;
      
      UpnpNode_free(UDN);
      UpnpNode_free(serviceList);
      
      return head;
    }
  else
    
    return NULL;
  
}



service_info * getAllServiceList(Upnp_Node node, char * URLBase)
{
  service_info * head=NULL;
  service_info * end=NULL;
  service_info *next_end=NULL;
  Upnp_NodeList deviceList=NULL;
  Upnp_Node currentDevice=NULL;
  
  int NumOfDevices=0;
  int i=0;

  deviceList=UpnpElement_getElementsByTagName(node, "device");
  if (deviceList!=NULL)
    {
      NumOfDevices=UpnpNodeList_getLength(deviceList);
      for (i=0;i<NumOfDevices;i++)
	  {
	    currentDevice=UpnpNodeList_item(deviceList,i);
	    if (head)
	      {
		end->next=getServiceList(currentDevice,&next_end,URLBase);
		end=next_end;
	      }
	    else
	      head=getServiceList(currentDevice,&end,URLBase);
	    UpnpNode_free(currentDevice);
	  }
      UpnpNodeList_free(deviceList);
    }
  return head;
}
 

int getServiceTable(Upnp_Node node,  
		    service_table * out,
		    char * DefaultURLBase)
{
  Upnp_Node root=NULL;
  Upnp_Node URLBase=NULL;
  //  Upnp_Node device;
  
  

  if (getSubElement("root",node,&root))
    { 
      if (getSubElement("URLBase",root,&URLBase))
	{
	  out->URLBase=getElementValue(URLBase);
	  UpnpNode_free(URLBase);
	}
      else 
	{ 
	  if (DefaultURLBase)
	    {
	      out->URLBase=UpnpCloneDOMString(DefaultURLBase);
	      
	    }
	  else
	    {
	      out->URLBase=UpnpCloneDOMString("");
	      (*out->URLBase)=0;
	    }
	}
     
      if ((out->serviceList=getAllServiceList(root,out->URLBase)))
	{
	  UpnpNode_free(root);
	  return 1;
	}
      UpnpNode_free(root);
    }

  return 0;
}
)
#endif

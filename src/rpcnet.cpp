// Copyright (c) 2009-2014 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "net.h"
#include "bitcoinrpc.h"
#include "main.h"

using namespace json_spirit;
using namespace std;

Value getconnectioncount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getconnectioncount\n"
            "Returns the number of connections to other nodes.");

    LOCK(cs_vNodes);
    return (int)vNodes.size();
}

Value getpeerinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpeerinfo\n"
            "Returns data about each connected network node.");

    vector<CNodeStats> vstats;
    {
        LOCK(cs_vNodes);
        vstats.reserve(vNodes.size());
        BOOST_FOREACH(CNode* pnode, vNodes) {
            CNodeStats stats;
            pnode->copyStats(stats);
            vstats.push_back(stats);
        }
    }

    Array ret;
    BOOST_FOREACH(const CNodeStats& stats, vstats) {
        Object obj;
        obj.push_back(Pair("addr", stats.addrName));
        obj.push_back(Pair("services", strprintf("%016"PRI64x, stats.nServices)));
        obj.push_back(Pair("lastsend", (boost::int64_t)stats.nLastSend));
        obj.push_back(Pair("lastrecv", (boost::int64_t)stats.nLastRecv));
        obj.push_back(Pair("bytessent", (boost::int64_t)stats.nSendBytes));
        obj.push_back(Pair("bytesrecv", (boost::int64_t)stats.nRecvBytes));
        obj.push_back(Pair("blocksrequested", (boost::int64_t)stats.nBlocksRequested));
        obj.push_back(Pair("conntime", (boost::int64_t)stats.nTimeConnected));
        obj.push_back(Pair("version", stats.nVersion));
        // Use the sanitized form of subver here, to avoid tricksy remote peers from
        // corrupting or modifiying the JSON output by putting special characters in
        // their ver message.
        obj.push_back(Pair("subver", stats.cleanSubVer));
        obj.push_back(Pair("inbound", stats.fInbound));
        obj.push_back(Pair("startingheight", stats.nStartingHeight));
        obj.push_back(Pair("banscore", stats.nMisbehavior));
        obj.push_back(Pair("syncnode", stats.fSyncNode));

        ret.push_back(obj);
    }

    return ret;
}

Value addnode(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() == 2)
        strCommand = params[1].get_str();
    if (fHelp || params.size() != 2 ||
        (strCommand != "onetry" && strCommand != "add" && strCommand != "remove"))
        throw runtime_error(
            "addnode <node> <add|remove|onetry>\n"
            "Attempts to add or remove a node from the addnode list.\n"
            "Or try a connection to a node once.");

    string strNode = params[0].get_str();

    if (strCommand == "onetry")
    {
        CAddress addr;
        ConnectNode(addr, strNode.c_str());
        return Value::null;
    }

    LOCK(cs_vAddedNodes);
    vector<string>::iterator it = vAddedNodes.begin();
    for(; it != vAddedNodes.end(); it++)
        if (strNode == *it)
            break;

    if (strCommand == "add")
    {
        if (it != vAddedNodes.end())
            throw JSONRPCError(RPC_CLIENT_NODE_ALREADY_ADDED, "Error: Node already added");
        vAddedNodes.push_back(strNode);
    }
    else if(strCommand == "remove")
    {
        if (it == vAddedNodes.end())
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_ADDED, "Error: Node has not been added.");
        vAddedNodes.erase(it);
    }

    return Value::null;
}

Value getaddednodeinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getaddednodeinfo <dns> [node]\n"
            "Returns information about the given added node, or all added nodes\n"
            "(note that onetry addnodes are not listed here)\n"
            "If dns is false, only a list of added nodes will be provided,\n"
            "otherwise connected information will also be available.");

    bool fDns = params[0].get_bool();

    list<string> lAddedNodes;
    if (params.size() == 1)
    {
        LOCK(cs_vAddedNodes);
        BOOST_FOREACH(string& strAddNode, vAddedNodes)
            lAddedNodes.push_back(strAddNode);
    }
    else
    {
        string strNode = params[1].get_str();
        LOCK(cs_vAddedNodes);
        BOOST_FOREACH(string& strAddNode, vAddedNodes)
            if (strAddNode == strNode)
                lAddedNodes.push_back(strAddNode);
        if (lAddedNodes.size() == 0)
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_ADDED, "Error: Node has not been added.");
    }

    if (!fDns)
    {
        Array ret;
        BOOST_FOREACH(string& strAddNode, lAddedNodes)
        {
            Object obj;
            obj.push_back(Pair("addednode", strAddNode));
            ret.push_back(obj);
        }
        return ret;
    }

    Array ret;

    list<pair<string, vector<CService> > > lAddedAddreses;
    BOOST_FOREACH(string& strAddNode, lAddedNodes)
    {
        vector<CService> vservNode;
        if(Lookup(strAddNode.c_str(), vservNode, GetDefaultPort(), fNameLookup, 0))
            lAddedAddreses.push_back(make_pair(strAddNode, vservNode));
        else
        {
            Object obj;
            obj.push_back(Pair("addednode", strAddNode));
            obj.push_back(Pair("connected", false));
            Array addresses;
            obj.push_back(Pair("addresses", addresses));
            ret.push_back(obj); // Added this line to push the object even if lookup fails
        }
    }

    BOOST_FOREACH(const PAIRTYPE(string, vector<CService>)& i, lAddedAddreses)
    {
        string strAddNode = i.first;
        vector<CService> vservNode = i.second;

        Object obj;
        obj.push_back(Pair("addednode", strAddNode));

        Array addresses;
        bool fConnected = false;
        BOOST_FOREACH(const CService& addrNode, vservNode)
        {
            bool fFound = false;
            Object node;
            node.push_back(Pair("address", addrNode.ToString()));
            {
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
                    if (pnode->addr == addrNode)
                    {
                        fFound = true;
                        fConnected = true;
                        node.push_back(Pair("connected", pnode->fInbound ? "inbound" : "outbound"));
                        break;
                    }
            }
            if (!fFound)
                node.push_back(Pair("connected", "false"));
            addresses.push_back(node);
        }
        obj.push_back(Pair("connected", fConnected));
        obj.push_back(Pair("addresses", addresses));
        ret.push_back(obj);
    }

    return ret;
}


static string GetNetworkName(enum Network net) {
    switch(net) {
    case NET_IPV4: return "ipv4";
    case NET_IPV6: return "ipv6";
    case NET_TOR: return "onion";
    default: return "unknown";
    }
}

Value getnetworkinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getnetworkinfo\n"
            "Returns an object containing various state info regarding P2P networking.\n"
            "\nResult:\n"
            "{\n"
            "  \"version\": xxxxx,                      (numeric) the server version\n"
            "  \"subversion\": \"/Linkcoin:x.x.x/\",     (string) the server subversion string\n"
            "  \"protocolversion\": xxxxx,              (numeric) the protocol version\n"
            "  \"localservices\": \"xxxxxxxxxxxxxxxx\", (string) the services we offer to the network\n"
            "  \"timeoffset\": xxxxx,                   (numeric) the time offset\n"
            "  \"connections\": xxxxx,                  (numeric) the number of connections\n"
            "  \"networks\": [                          (array) information per network\n"
            "  {\n"
            "    \"name\": \"xxx\",                     (string) network (ipv4, ipv6 or onion)\n"
            "    \"limited\": true|false,               (boolean) is the network limited using -onlynet?\n"
            "    \"reachable\": true|false,             (boolean) is the network reachable?\n"
            "    \"proxy\": \"host:port\"               (string) the proxy that is used for this network, or empty if none\n"
            "  }\n"
            "  ,...\n"
            "  ],\n"
            "  \"relayfee\": x.xxxx,                    (numeric) minimum relay fee for non-free transactions in LTC/KB\n"
            "  \"localaddresses\": [                    (array) list of local addresses\n"
            "  {\n"
            "    \"address\": \"xxxx\",                 (string) network address\n"
            "    \"port\": xxx,                         (numeric) network port\n"
            "    \"score\": xxx                         (numeric) relative score\n"
            "  }\n"
            "  ,...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            "> linkcoin-cli getnetworkinfo\n"
            "> curl --user myusername --data-binary '{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", \"method\": \"getnetworkinfo\", \"params\": [] }' -H 'content-type: text/plain;' http://127.0.0.1:9600/\n"
        );

    Object obj;
    obj.push_back(Pair("version",       (int)CLIENT_VERSION));
    obj.push_back(Pair("subversion",    FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, std::vector<std::string>())));
    obj.push_back(Pair("protocolversion",(int)PROTOCOL_VERSION));
    obj.push_back(Pair("localservices",       strprintf("%016"PRI64x, nLocalServices)));
    obj.push_back(Pair("timeoffset",    (boost::int64_t)GetTimeOffset()));
    obj.push_back(Pair("connections",   (int)vNodes.size()));

    Array networks;
    for(int n=0; n<NET_MAX; n++) {
        enum Network network = (enum Network)n;
        if(network == NET_UNROUTABLE || network == NET_TOR) continue;
        proxyType proxy;
        Object objNet;
        objNet.push_back(Pair("name", GetNetworkName(network)));
        objNet.push_back(Pair("limited", IsLimited(network)));
        objNet.push_back(Pair("reachable", IsReachable(network)));
        objNet.push_back(Pair("proxy", GetProxy(network, proxy) ? proxy.first.ToStringIPPort() : string()));
        objNet.push_back(Pair("proxy_randomize_credentials", false));
        networks.push_back(objNet);
    }
    obj.push_back(Pair("networks",      networks));
    obj.push_back(Pair("relayfee",      ValueFromAmount(nTransactionFee)));
    Array local_addresses;
    // mapLocalHost is not exposed, skipping localaddresses
    obj.push_back(Pair("localaddresses", local_addresses));
    return obj;
}

Value getnetworkstats(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getnetworkstats\n"
            "Returns statistics about network traffic.\n"
            "\nResult:\n"
            "{\n"
            "  \"totalbytesrecv\": n,   (numeric) Total bytes received\n"
            "  \"totalbytessent\": n,   (numeric) Total bytes sent\n"
            "}\n"
            "\nExamples:\n"
            "> linkcoin-cli getnetworkstats \n"
            "> curl --user myusername --data-binary '{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", \"method\": \"getnetworkstats\", \"params\": [] }' -H 'content-type: text/plain;' http://127.0.0.1:9600/\n"
        );

    int64_t nTotalBytesRecv = 0;
    int64_t nTotalBytesSent = 0;
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes) {
            nTotalBytesRecv += pnode->nRecvBytes;
            nTotalBytesSent += pnode->nSendBytes;
        }
    }

    Object obj;
    obj.push_back(Pair("totalbytesrecv", (boost::int64_t)nTotalBytesRecv));
    obj.push_back(Pair("totalbytessent", (boost::int64_t)nTotalBytesSent));
    return obj;
}

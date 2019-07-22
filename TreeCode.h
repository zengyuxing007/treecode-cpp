#ifndef _TREE_CODE_H__
#define _TREE_CODE_H__

#include <vector>
#include <string>
#include <assert.h>
#include <fstream>
#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>
#include <stdint.h>
#include <iostream>
#include "nType.h"
#include "BinaryReader.h"
#include "Stream.h"
#include "BufferType.h"

extern "C"
{
#include "ant/log.h"
}


using namespace std;


#define MAX_BUFF_LEN 20480

typedef unsigned char byte;	

/*Binary Extendable Code自定义的文件格式，二进制可扩展编码格式
  文件是树形结构，每个节点都可以添加无限的子节点
  节点的结构：
  节点名称长度-byte
  名称-UTF8数组
  节点类型-byte,一个类型枚举,可以表示 bool,char,uchar,short,ushort,int32,uint32,int64,uint64,float,double,utf8string,buffer,float3,float2


  节点内容-基本类型直接写内容本身；buffer－uint缓冲长度+缓冲本身
  节点后面紧跟子节点的数量-ushort
  数量后面紧跟子节点的信息
  */


class Node
{
    friend class TreeCode;
    public:
    enum TypeCode
    {
        // 摘要:
        //     空引用。
        Empty = 0,
        //
        // 摘要:
        //     常规类型，表示不会由另一个 TypeCode 显式表示的任何引用或值类型。
        Object = 1,
        //
        // 摘要:
        //     数据库空（列）值。
        DBNull = 2,
        //
        // 摘要:
        //     简单类型，表示 true 或 false 的布尔值。
        Boolean = 3,
        //
        // 摘要:
        //     整型，表示值介于 0 到 65535 之间的无符号 16 位整数。System.TypeCode.Char 类型的可能值集与 Unicode 字符集相对应。
        WChar = 4,
        //
        // 摘要:
        //     整型，表示值介于 -128 到 127 之间的有符号 8 位整数。
        SByte = 5,
        //
        // 摘要:
        //     整型，表示值介于 0 到 255 之间的无符号 8 位整数。
        Byte = 6,
        //
        // 摘要:
        //     整型，表示值介于 -32768 到 32767 之间的有符号 16 位整数。
        Int16 = 7,
        //
        // 摘要:
        //     整型，表示值介于 0 到 65535 之间的无符号 16 位整数。
        UInt16 = 8,
        //
        // 摘要:
        //     整型，表示值介于 -2147483648 到 2147483647 之间的有符号 32 位整数。
        Int32 = 9,
        //
        // 摘要:
        //     整型，表示值介于 0 到 4294967295 之间的无符号 32 位整数。
        UInt32 = 10,
        //
        // 摘要:
        //     整型，表示值介于 -9223372036854775808 到 9223372036854775807 之间的有符号 64 位整数。
        Int64 = 11,
        //
        // 摘要:
        //     整型，表示值介于 0 到 18446744073709551615 之间的无符号 64 位整数。
        UInt64 = 12,
        //
        // 摘要:
        //     浮点型，表示从大约 1.5 x 10 -45 到 3.4 x 10 38 且精度为 7 位的值。
        Single = 13,
        //
        // 摘要:
        //     浮点型，表示从大约 5.0 x 10 -324 到 1.7 x 10 308 且精度为 15 到 16 位的值。
        Double = 14,
        //
        // 摘要:
        //     简单类型，表示从 1.0 x 10 -28 到大约 7.9 x 10 28 且有效位数为 28 到 29 位的值。
        Decimal = 15,
        //
        // 摘要:
        //     表示一个日期和时间值的类型。
        DateTime = 16,				
        //
        // 摘要:
        //     密封类类型，表示 Unicode 字符串。
        UnicodeString = 18,
        //
        // 摘要:
        //     密封类类型，表示 UTF8 字符串。
        UTF8String = 19,				
        //
        // 摘要:
        //     一个内存缓冲。
        Buffer = 20,
        //
        // 摘要:
        //     float2。
        Vector2 = 21,
        //
        // 摘要:
        //     float3
        Vector3 = 22,
        //
        Pos2 = 25,
        //
    };
    ~Node()
    {		
        if(type == Buffer)
        {
           buffer_t buff=boost::any_cast<buffer_t>(obj);				
           if(buff.type == 0 && buff.len > 0 && buff.p != NULL){
               byte* p = (byte*)buff.p;
               delete[] p;
               buff.p = NULL;
               buff.len = 0;
           }

        }
        for (unsigned int i=0;i<sons.size();i++)
        {
            delete(sons[i]);
        }
    }


    void setEmptyObj()
    {
        type = Empty;
        this->obj = string("null");
    }

    void setObj(const char* obj)
    {
        setObj(string(obj));
    }

    void setObj(char obj){
        type = SByte;
        this->obj = obj;
    }

    void setObj(unsigned char obj){
        //printf("set unsigned char---%u\n",obj);
        type = Byte;
        this->obj = obj;
    }

    template<typename T>
        void setObj(const T& obj)
        {
            type = GetTypeCode(obj);			
            this->obj = obj;
        }

    static string readString(istream& stream)
    {
        unsigned int nameLen=0;
        stream.read((char*)&nameLen,sizeof(nameLen));			

        char* p=new char[nameLen+1];				
        stream.read((char*)p,nameLen);			
        p[nameLen]=0;
        string str=p;
        delete[] p;
        p = NULL;
        /*string result;
          utf8ToUnicode(str,result);*/
        return str;
    }		
    static void writeString(ostream& stream,const string& str)
    {
        unsigned int tmp=str.size();
        stream.write((char*)&tmp,sizeof(tmp));			
        stream.write((char*)str.c_str(),str.size());
    }

    static void writeString(Stream& stream,const string& str)
    {
        unsigned int tmp=str.size();
        stream.write((char*)&tmp,sizeof(tmp));			
        stream.write((char*)str.c_str(),str.size());
    }

    private:
#define GET_TYPE(T,E) TypeCode GetTypeCode(T v){return E;}
    GET_TYPE(bool,Boolean)	GET_TYPE(char,Byte)	GET_TYPE(byte,Byte)	GET_TYPE(short,Int16)	GET_TYPE(unsigned short,UInt16)	GET_TYPE(int,Int32)	
        GET_TYPE(unsigned int,UInt32)	GET_TYPE(int64_t,Int64)	GET_TYPE(uint64_t,UInt64)	GET_TYPE(float,Single)	GET_TYPE(double,Double)	
        GET_TYPE(const string&,UTF8String)	GET_TYPE(buffer_t,Buffer)	GET_TYPE(float2,Vector2)	GET_TYPE(float3,Vector3) GET_TYPE(pos2,Pos2)



        void load(istream& stream)
        {		
            byte nameLen=0;
            stream.read((char*)&nameLen,sizeof(nameLen));			

            char* p=new char[nameLen+1];				
            stream.read((char*)p,nameLen);			
            p[nameLen]=0;
            name=p;
            delete[] p;			
            p= NULL;

            byte tmpByte;
            stream.read((char*)&tmpByte,sizeof(tmpByte));			
            type=(TypeCode)tmpByte;

            //根据类型采取不同读取方式
#define ISTREAM_READ_TYPE(T) {T tmp;stream.read((char*)&tmp,sizeof(tmp));obj=tmp;}
            switch(type)
            {				
                case Empty:     obj=string("null"); break;
                case Boolean:				ISTREAM_READ_TYPE(bool);				break;
                case WChar:				ISTREAM_READ_TYPE(unsigned short);				break;
                case Byte:				ISTREAM_READ_TYPE(unsigned char);				break;
                case SByte:				ISTREAM_READ_TYPE(unsigned char);				break;
                case Int16:				ISTREAM_READ_TYPE(short);				break;
                case UInt16:				ISTREAM_READ_TYPE(unsigned short);				break;
                case Int32:				ISTREAM_READ_TYPE(int);				break;
                case UInt32:				ISTREAM_READ_TYPE(unsigned int);				break;
                case Int64:				ISTREAM_READ_TYPE(int64_t);				break;
                case UInt64:				ISTREAM_READ_TYPE(uint64_t);				break;
                case Single:				ISTREAM_READ_TYPE(float);				break;
                case Double:				ISTREAM_READ_TYPE(double);				break;
                case UTF8String:				obj=readString(stream);						break;				
                case Buffer:
                                                {
                                                buffer_t buff;				
                                                stream.read((char*)&buff.len,sizeof(buff.len));						
                                                buff.p=new byte[buff.len];
                                                stream.read((char*)buff.p,buff.len);						
                                                obj=buff;
                                                }
                                                break;
                case Vector2:				ISTREAM_READ_TYPE(float2);				break;
                case Vector3:				ISTREAM_READ_TYPE(float3);				break;
                case Pos2:                  ISTREAM_READ_TYPE(pos2);   break;
                default:	
#ifdef DEBUG_PRINT
                                            printf("not expected typecode:%d\n",type);
#endif
                                            assert(!"not expected typecode");
                                            break;
            }		

            unsigned short num = 0;
            stream.read((char*)&num,sizeof(num));			
            //printf("node[%s] have sons[%u]\n",name.c_str(),num);
            for (unsigned short i = 0; i < num; i++)
            {
                Node* n = new Node();
                sons.push_back(n);
                n->parent = this;
                n->load(stream);
            }
        }



        void load(void* data,unsigned int len)
        {
            BinaryReader stream(data,len,false);
            load(stream);
        }

        //load
        void load(BinaryReader& stream)
        {
            UInt8 nameLen = 0;
            UInt8 intType = 0;
            stream >> nameLen;
            name.resize(nameLen);
            stream.read(name,nameLen);
            //printf("###1:pos:%u\n",stream.pos());
            stream >> intType;
            type = (TypeCode) intType;
            //printf("###2:pos:%u\n",stream.pos());

#define READ_TYPE(T) {T tmp; stream >> tmp; obj = tmp;}

            switch(type)
            {				
                case Empty:            
             //       printf("empty---------\n");
                    obj=string("null"); break;
                case Boolean:		   READ_TYPE(bool);				break;
                case WChar:			   READ_TYPE(unsigned short);				break;
                case Byte:			   READ_TYPE(unsigned char);				break;
                case SByte:			   READ_TYPE(unsigned char);				break;
                case Int16:			   READ_TYPE(short);				break;
                case UInt16:		   READ_TYPE(unsigned short);				break;
                case Int32:			   READ_TYPE(int);				break;
                case UInt32:		   READ_TYPE(unsigned int);				break;
                case Int64:			   READ_TYPE(int64_t);				break;
                case UInt64:		   READ_TYPE(uint64_t);				break;
                case Single:		   READ_TYPE(float);				break;
                case Double:		   READ_TYPE(double);				break;
                case UTF8String:	   
                                       {
                                           std::string tmp;
                                           stream.readString32(tmp);
                                           obj = tmp;
                                       }
                                       break;				
                case Buffer:           
                                       {
                                           buffer_t tmp;				
                                           stream.read((unsigned char*)&tmp.len,sizeof(tmp.len));						
                                           if(tmp.len > MAX_BUFF_LEN or tmp.len < 1)
                                           {
                                               ERROR_LOG("buffer len > MAX_BUFF(%u) or buffer is null,len is [%u]",MAX_BUFF_LEN,tmp.len);
                                               DEBUG_LOG("buffer len > MAX_BUFF(%u) or buffer is null,len is [%u]",MAX_BUFF_LEN,tmp.len);
                                               tmp.len = 0;
                                               tmp.p = NULL;
                                               obj = tmp;
                                               break;
                                           }
                                           tmp.p=new byte[tmp.len];
                                           stream.read((unsigned char*)tmp.p,tmp.len);						
                                           this->obj=tmp;

                                           buffer_t a;
                                           a = boost::any_cast<buffer_t>(obj);
                                           //DEBUG_LOG("tree read buffer,buff_len[%u]",a.len);
                                       }
                                       break;
                case Vector2:		   READ_TYPE(float2);				break;
                case Vector3:		   READ_TYPE(float3);				break;
                case Pos2:		       READ_TYPE(pos2);				break;
                default:	
#ifdef DEBUG_PRINT
                                       DEBUG_LOG("not expected typecode:%d",type);
#endif
                                       //assert(!"not expected typecode");
                                       break;
            }
            unsigned short num = 0;
            stream >> num;
            //num = ntohs(num);
            //printf("pos[%u],node[%s]---sons num:[%u]\n",stream.pos(),name.c_str(),num);
            for (unsigned short i = 0; i < num; i++)
            {
                Node* n = new Node();
                sons.push_back(n);
                n->parent = this;
                n->load(stream);
            }
        }

    void save(ostream& stream)
    {
        assert(((name.size() < 65536) || !"node's name is too long to save name"));

        byte tmp=name.size();
        stream.write((char*)&tmp,sizeof(tmp));			
        stream.write((char*)name.c_str(),name.size());

        byte tmpByte=type;
        stream.write((char*)&tmpByte,sizeof(tmpByte));

        //根据类型采取不同写入方式
        buffer_t buff;
#define WRITE_TYPE(T) {T tmp=boost::any_cast<T>(obj);stream.write((char*)&tmp,sizeof(tmp));}
        switch(type)
        {				
            case Empty:              break; //write nothing
            case Boolean:				WRITE_TYPE(bool);				break;
            case WChar:				WRITE_TYPE(unsigned short);				break;
            case Byte:				WRITE_TYPE(unsigned char);				break;
            case SByte:				WRITE_TYPE(unsigned char);				break;
            case Int16:				WRITE_TYPE(short);				break;
            case UInt16:				WRITE_TYPE(unsigned short);				break;
            case Int32:				WRITE_TYPE(int);				break;
            case UInt32:				WRITE_TYPE(unsigned int);				break;
            case Int64:				WRITE_TYPE(int64_t);				break;
            case UInt64:				WRITE_TYPE(uint64_t);				break;
            case Single:				WRITE_TYPE(float);				break;
            case Double:				WRITE_TYPE(double);				break;
            case UTF8String:				writeString(stream,boost::any_cast<string>(obj));						break;				
            case Buffer:
                                            {
                                            buff=boost::any_cast<buffer_t>(obj);				
                                            stream.write((char*)&buff.len,sizeof(buff.len));										
                                            if(buff.len > 0 && buff.p != NULL)
                                                stream.write((char*)buff.p,buff.len);
                                            }
                                            break;
            case Vector2:				WRITE_TYPE(float2);				break;
            case Vector3:				WRITE_TYPE(float3);				break;
            case Pos2:				WRITE_TYPE(pos2);				break;
            default:	
#ifdef DEBUG_PRINT
                                        printf("not expected typecode:%d\n",type);
#endif
                                        assert(!"not expected typecode");
                                        break;
        }								

        unsigned short sonNum=(unsigned short)sons.size();
        stream.write((char*)&sonNum,sizeof(sonNum));

        for (unsigned int i=0;i<sons.size();i++)
        {
            sons[i]->save(stream);
        }
    }


    void save(Stream& stream)
    {
        assert(((name.size() < 65536) || !"node's name is too long to save name"));

        byte tmp=name.size();
        stream.write((char*)&tmp,sizeof(tmp));			
        stream.write((char*)name.c_str(),name.size());

        byte tmpByte=type;
        stream.write((char*)&tmpByte,sizeof(tmpByte));

        //根据类型采取不同写入方式
        buffer_t buff;
#define WRITE_TYPE(T) {T tmp=boost::any_cast<T>(obj);stream.write((char*)&tmp,sizeof(tmp));}
        switch(type)
        {				
            case Empty:              break; //write nothing
            case Boolean:				WRITE_TYPE(bool);				break;
            case WChar:				WRITE_TYPE(unsigned short);				break;
            case Byte:				WRITE_TYPE(unsigned char);				break;
            case SByte:				WRITE_TYPE(unsigned char);				break;
            case Int16:				WRITE_TYPE(short);				break;
            case UInt16:				WRITE_TYPE(unsigned short);				break;
            case Int32:				WRITE_TYPE(int);				break;
            case UInt32:				WRITE_TYPE(unsigned int);				break;
            case Int64:				WRITE_TYPE(int64_t);				break;
            case UInt64:				WRITE_TYPE(uint64_t);				break;
            case Single:				WRITE_TYPE(float);				break;
            case Double:				WRITE_TYPE(double);				break;
            case UTF8String:				writeString(stream,boost::any_cast<string>(obj));						break;				
            case Buffer:
                                            buff=boost::any_cast<buffer_t>(obj);				
                                            stream.write((char*)&buff.len,sizeof(buff.len));										
                                            if(buff.len > 0 && buff.p != NULL)
                                                stream.write((char*)buff.p,buff.len);
                                            break;
            case Vector2:				WRITE_TYPE(float2);				break;
            case Vector3:				WRITE_TYPE(float3);				break;
            case Pos2:				WRITE_TYPE(pos2);				break;
            default:	
#ifdef DEBUG_PRINT
                                        printf("not expected typecode:%d\n",type);
#endif
                                        assert(!"not expected typecode");
                                        break;
        }								

        unsigned short sonNum=(unsigned short)sons.size();
        stream.write((char*)&sonNum,sizeof(sonNum));

        for (unsigned int i=0;i<sons.size();i++)
        {
            sons[i]->save(stream);
        }
    }



    string printAny()
    {
        int len;float2 v2;float3 v3; pos2 pos;
#define ANY_TOSTRING(T) boost::lexical_cast<string>(boost::any_cast<T>(obj))
        switch(type)
        {
            case Empty:
                return ANY_TOSTRING(string);
            case Boolean:
                return boost::any_cast<bool>(obj) ? "true":"false";
            case WChar:
                return ANY_TOSTRING(unsigned short);
            case Byte:
                {
                    unsigned short t = boost::any_cast<unsigned char>(obj);
                    return boost::lexical_cast<string>(t);
                }
            case Int16:
                return ANY_TOSTRING(short);
            case UInt16:
                return ANY_TOSTRING(unsigned short);
            case Int32:
                return ANY_TOSTRING(int);
            case UInt32:
                return ANY_TOSTRING(unsigned int);
            case Int64:
                return ANY_TOSTRING(int64_t);
            case UInt64:
                return ANY_TOSTRING(uint64_t);
            case Single:
                return ANY_TOSTRING(float);
            case Double:
                return ANY_TOSTRING(double);
            case UTF8String:
                return ANY_TOSTRING(string);
            case Buffer:
                len=boost::any_cast<buffer_t>(obj).len;
                return string("buffer[")+boost::lexical_cast<string>(len)+"]";
                break;
            case Vector2:
                v2=boost::any_cast<float2>(obj);
                return string("vector2[")+boost::lexical_cast<string>(v2.x)+","+boost::lexical_cast<string>(v2.y)+"]";
                break;
            case Vector3:
                v3=boost::any_cast<float3>(obj);
                return string("vector3[")+boost::lexical_cast<string>(v3.x)+","+boost::lexical_cast<string>(v3.y)+","+boost::lexical_cast<string>(v3.z)+"]";
                break;
            case Pos2:
                pos=boost::any_cast<pos2>(obj);
                return string("pos2[")+boost::lexical_cast<string>(pos.x)+","+boost::lexical_cast<string>(pos.y)+"]";
                break;
                
            default:
                /*do nothing */
                break;
        };
        return string(""); //return null string
    }

    private:
//    public:
    string name;//节点名称	
    TypeCode type;//类型序号
    boost::any obj;//内容
    vector<Node*> sons;//子节点
    Node* parent;//父节点
};


class TreeCode
{
    public:
        TreeCode():focusNode(NULL),rootNode(NULL)
        {

        }

        ~TreeCode()
        {
            if(rootNode != NULL){
                delete(rootNode);
            }
        }

        TreeCode(const string& name):focusNode(NULL),rootNode(NULL)
        {
            addEmptyNode(name);
        }

        Node* addEmptyNode(const string& name)
        {
            Node* node = new Node();
            if (rootNode == NULL)
            {
                focusNode = rootNode = node;
            }
            else
            {
                focusNode->sons.push_back(node);
                node->parent = focusNode;
            }
            node->name = name;
            node->setEmptyObj();
            return node;
        }

        Node* addRetCode(UInt16 returnValue )
        {
            return addNode("retcode",returnValue);
        }

        //在当前节点下面添加一个子节点,isChangeFocus 是否改变当前节点指针
        template<typename T>
            Node* addNode(const string& name, const T& v,bool isChangeFocus=false)
            {
                Node* node = new (std::nothrow) Node();
                if (rootNode == NULL)
                {
                    focusNode = rootNode = node;
                }
                else
                {
                    //printf("addNode,focusNode[%s],node[%s]\n",focusNode->name.c_str(),name.c_str());
                    focusNode->sons.push_back(node);
                    node->parent = focusNode;
                }
                node->name = name;
                node->setObj(v);
                if(isChangeFocus){
                    focusNode = node;
                }

                return node;
            }
        /*写入子节点，如果没有则临时创建，有了直接写入,不改变当前节点指针
          \return 是否创建新节点
          */
        template<typename T>
            bool writeSon(const string& sonname, const T& t)
            {
                if (getSon(sonname, false) != NULL)
                {
                    focusNode->setObj(t);
                    toParent();
                    return false;
                }
                else
                {
                    addNode(sonname, t);
                    return true;
                }
            }
        //回到上层节点，\param name 确认节点名称正确
        Node* toParent(const string& name)
        {
            //assert(focusNode != rootNode);
            if(focusNode == rootNode){
                return focusNode;
            }
            if (!name.empty())
            {
                if(focusNode->parent->name != name)
                {
                    string msg=string("不是预期的父节点 name:")+name+" 实际:"+focusNode->parent->name;
                    assert(!msg.c_str());
                }
            }               
            return focusNode = focusNode->parent;
        }

        //回到上层节点
        Node* toParent()
        {
            return toParent("");
        }
        //得到子节点数量
        int getSonNum()
        {
            return focusNode->sons.size();
        }
        //得到子节点
        Node* getSon(unsigned int i)
        {
            assert(i + 1 <= focusNode->sons.size());
            return focusNode = focusNode->sons[i];
        }
        /*得到子节点
          \boolAssert 找不到时是否assert
          */
        Node* getSon(const string& name, bool boolAssert=false)
        {
            for (unsigned int i=0;i<focusNode->sons.size();i++)
            {
                if (focusNode->sons[i]->name == name)                
                    return focusNode = focusNode->sons[i];                
            }
            if (boolAssert)
                assert(!(string("cant' find node :") + name).c_str());

            return NULL;
        }
        //得到最后一个子节点，当AddNode后可以这样得到刚添加的新节点
        Node* getLastSon()
        {
            return focusNode = focusNode->sons[focusNode->sons.size() - 1];
        }
        //尝试读取一个子节点的内容并不改变当前节点指针
        template<typename T>
            bool readSon(const string& sonname, T& t)
            {
                if (getSon(sonname, false) != NULL)
                {
                    if(!read(t)){
                        toParent();
                        return false;
                    }else{
                        //DEBUG_LOG("read son[%s] ok",sonname.c_str());
                        toParent();
                        return true;
                    }
                }
                DEBUG_LOG("read son[%s] failed, not found",sonname.c_str());
                INFO_LOG("read son[%s] failed, not found",sonname.c_str());
                return false;
            }     
        /*通过路径名得到子节点
          \boolAssert 找不到时是否assert
          */		
        Node* findNode(const string& path, bool boolAssert=false)
        {
            try
            {			
                vector<string> names =split(path,"/");
                if (path[0]=='/')
                    focusNode = rootNode;
                for (unsigned int i=0;i<names.size();i++)
                {
                    const string& name=names[i];

                    if (getSon(name, boolAssert) == NULL)
                    {
                        throw "can't find this node";
                    }

                }
            }
            catch (...)
            {
                if (boolAssert)
                    assert(0 && "can't find this node");
                return NULL;
            }
            return focusNode;
        }

        //读取当前节点的值
        template<typename T>
            bool read(T& t)
            {
                try
                {
                    t = boost::any_cast<T>(focusNode->obj);
                }			
                catch(...)
                {
                    DEBUG_LOG("read node[%s]----type[%u],catch error....",focusNode->name.c_str(),focusNode->type);
                    ERROR_LOG("read node[%s]----type[%u],catch error....",focusNode->name.c_str(),focusNode->type);
                    return false;
                }
                return true;
            }
        //读取当前节点的值,当节点是一个buffer时使用
        void read(void*& p,unsigned int& len)
        {
            buffer_t buff =boost::any_cast<buffer_t>(focusNode->obj);
            p=buff.p;
            len=buff.len;
        }
        //得到当前节点的名称
        string getName()
        {
            return focusNode->name;
        }
        /*
        //从文件载入
        bool load(const string& filename)
        {
        rootNode = focusNode = new Node();
        ifstream inFile(filename.c_str());

        //assert(inFile.is_open());
        if(!inFile.is_open())
        return false;

        rootNode->load(inFile);

        focusNode = rootNode;
        inFile.close();
        return true;
        }       
        //从一个内存流载入
        void load(istream& stream)
        {
        stream.seekg(ios::beg);			
        rootNode = focusNode = new Node();
        rootNode->load(stream);
        focusNode = rootNode;
        }
        */

        void load(void* data,UInt32 len,UInt32 offset)
        {
            void* ptr = (char*)data + offset;
            UInt32 nowLen = len - offset;
            load(ptr,nowLen);
        }

        //从一段内存中载入
        void load(void* data,UInt32 len)
        {
            rootNode = focusNode = new Node();
            rootNode->load(data,len);
            focusNode = rootNode;
        }
        //保存到文件
        void save(const string& filename)
        {
            ofstream outFile(filename.c_str());			
            rootNode->save(outFile);
            outFile.close();
        }
        void out(Stream& st)
        {
            rootNode->save(st);
        }
        void dump()
        {
            DEBUG_LOG("treecodeDump start:###########################################");
            string log;
            print(log,rootNode,0);
            DEBUG_LOG("%s",log.c_str());
            DEBUG_LOG("treecodeDump end:###########################################");
        }
        string print()
        {
            string log;
            print(log,rootNode,0);
            return log;
        }
        void toString(string out)
        {
            out.clear();
            print(out,rootNode,0);
        }
        void print(string& log,Node* p,size_t level)
        {				
            string space="	";
            for (size_t i=0;i<level;i++)				log+=space;
            log+=p->name+"="+p->printAny();			
            if(!p->sons.empty())
            {
                log+="\n";
                for (size_t i=0;i<level;i++)				log+=space;
                log+="|\n";
                for(size_t i=0;i<p->sons.size();i++)
                {
                    print(log,p->sons[i],level+1);
                    log+="\n";
                }
            }

        }

    private:		
        vector<string> split(const string& str,const char* c)
        {
            char *cstr, *p;
            vector<string> res;
            cstr = new char[str.size()+1];			
            strcpy(cstr,str.c_str());
            p = strtok(cstr,c);
            while(p!=NULL)
            {
                res.push_back(p);
                p = strtok(NULL,c);
            }
            return res;
        }
    private:
        Node* focusNode;
        Node* rootNode;

};

#endif

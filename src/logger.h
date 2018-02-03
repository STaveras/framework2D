
#pragma once

#include <fstream>
#include <string>

class Logger
{
    bool _isNew;
    std::ofstream* _pFileStream;
    std::string _szFilename;

protected:
    std::string _formatLogText(const char* text) const
    {
        std::string FormattedText = (_isNew) ? "" : "\n";
        FormattedText += text;
        return FormattedText;
    }

    bool _createFileStream(void)
    { 
        if (_pFileStream)
            return true;
        else if (_szFilename != "")
        {
            _pFileStream = new std::ofstream();
            return true;
        }
        
        return false;
    }

public:
    Logger(void):_isNew(true),_szFilename(""),_pFileStream(NULL){}
    Logger(const char* filename):_isNew(true),_szFilename(filename), _pFileStream(NULL){}
    ~Logger(void){ if (_pFileStream) { _pFileStream->clear(); delete _pFileStream; } }    

    //void close(void)
    //{
    //    if (_pFileStream)
    //    {
    //        if (_pFileStream->is_open())
    //        {
    //            _pFileStream->flush();
    //            _pFileStream->close();
    //            _pFileStream->clear();
    //        }
    //    }
    //}

    //void open(const char* szFilename)
    //{
    //    if (_szFilename != szFilename)
    //        close();
    //    else
    //    {
    //        _szFilename = szFilename;
    //        _createFileStream();
    //    }
    //}

    virtual void write(const char* entry)
    {
        // TODO: Write formatted timestamp
        if (_createFileStream())
        {
            std::string formattedString = _formatLogText(entry);

            _pFileStream->open(_szFilename, std::ios_base::app);
            _pFileStream->write(formattedString.c_str(), formattedString.size());
            _pFileStream->close();

            _isNew = false;
        }
    }

    virtual void clear(void)
    {
        if (_createFileStream())
        {
            _pFileStream->open(_szFilename, std::ios_base::trunc);
            _pFileStream->close();
            _isNew = true;
        }
    }
}; 
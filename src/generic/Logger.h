




#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <mutex>
#include <memory>

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    None = 4
};

// Un buffer qui duplique l'écriture vers deux flux distincts (Console + Fichier)
class TeeBuf : public std::streambuf {
private:
    std::streambuf* sb1_;
    std::streambuf* sb2_;

protected:
    virtual int overflow(int c) override;
    virtual int sync() override;

public:
    TeeBuf(std::streambuf* sb1, std::streambuf* sb2);
};

class Logger {
private:
    std::ofstream logFile_;
    std::unique_ptr<TeeBuf> teeBuf_;
    std::unique_ptr<std::ostream> logStream_;
    std::unique_ptr<std::ostream> nullStream_;
    LogLevel currentLevel_;
    std::mutex mutex_;

    Logger();
    ~Logger();

public:
    static Logger& GetInstance();

    // Configuration
    void SetLevel(LogLevel level);
    bool EnableFile(const std::string& filename);

    // Retourne le flux actif (ou un flux vide si le niveau est masqué)
    std::ostream& GetStream(LogLevel level, const char* prefix);
    
    // Pour accéder au mutex si besoin d'isoler une ligne multi-chevrons complète
    std::mutex& GetMutex();
};

// --- MACROS GLOBALES DE REMPLACEMENT ---
// Tu peux remplacer directement tes std::cout par LOG_DEBUG ou LOG_INFO 
// et garder exactement la même syntaxe avec <<
#define LOG_DEBUG Logger::GetInstance().GetStream(LogLevel::Debug, "[DEBUG]")
#define LOG_INFO  Logger::GetInstance().GetStream(LogLevel::Info,  "[INFO] ")
#define LOG_WARN  Logger::GetInstance().GetStream(LogLevel::Warning, "[WARN] ")
#define LOG_ERROR Logger::GetInstance().GetStream(LogLevel::Error, "[ERROR]")









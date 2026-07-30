#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <mutex>
#include <memory>

//
//
//
//    EXEMPLE d'utilistaion
//
//
//    // ON DÉFINIT LE NIVEAU LOCAL AVANT D'INCLURE (ou juste avant les logs)
//    #define LOCAL_LOG_LEVEL LogLevel::Warning
//    #include "Logger.h"
//
//        void InterfaceUI::rafraichir() {
//        LOG_DEBUG << "Redessin de la fenêtre"; // NE S'AFFICHERA PAS (filtré par le fichier)
//        LOG_WARN  << "Ralentissement détecté UI";   // S'affichera
//    }
//
//
//
//    // 1. On définit une macro par défaut pour le fichier si elle n'existe pas déjà
//    #ifndef LOCAL_LOG_LEVEL
//    #define LOCAL_LOG_LEVEL ::LogLevel::Debug // Niveau par défaut si le fichier ne précise rien
//    #endif
//




// 2. Logique de filtrage :
// Un message s'affiche si :
//   - Son niveau est >= au niveau local du fichier (défini par le .cpp)
//   ET
//   - Son niveau est >= au niveau global de l'application (contrôlé par Logger::GetInstance().SetLevel())
#include <algorithm> // pour std::max si besoin, ou simple opérateur ternaire

// Astuce : On demande au logger le niveau global, et on prend le max entre le global et le local du fichier,
// ou on gère le filtre directement dans la macro :

#define SHOULD_LOG(msgLevel) ( (msgLevel >= LOCAL_LOG_LEVEL) && (msgLevel >= Logger::GetInstance().GetGlobalLevel()) )

// Modification des macros de flux :
//










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


// Si le fichier .cpp n'a pas défini son propre niveau, on utilise une valeur par défaut (par exemple -1 ou une valeur sentinelle),
// ou on s'appuie directement sur le niveau global du Logger.
// Plus proprement : on définit un niveau par défaut global dans la classe Logger.
class Logger {
private:
    std::ofstream logFile_;
    std::unique_ptr<TeeBuf> teeBuf_;
    std::unique_ptr<std::ostream> logStream_;
    std::unique_ptr<std::ostream> nullStream_;
    LogLevel globalLevel_;
    std::mutex mutex_;

    Logger();
    ~Logger();

public:
    static Logger& GetInstance();
    LogLevel GetGlobalLevel() const;

    // Configuration
    void SetLevel(LogLevel level);
    bool EnableFile(const std::string& filename);

    std::ostream& GetStream(LogLevel messageLevel, LogLevel fileLevel, const char* prefix);
    
    // Pour accéder au mutex si besoin d'isoler une ligne multi-chevrons complète
    std::mutex& GetMutex();
};

// --- MACROS GLOBALES DE REMPLACEMENT ---
// Tu peux remplacer directement tes std::cout par LOG_DEBUG ou LOG_INFO 
// et garder exactement la même syntaxe avec <<


//  On définit une macro par défaut pour le fichier si elle n'existe pas déjà
#ifndef LOCAL_LOG_LEVEL
#define LOCAL_LOG_LEVEL ::LogLevel::Debug // Niveau par défaut si le fichier ne précise rien
#endif


#define LOG_DEBUG Logger::GetInstance().GetStream(LogLevel::Debug, LOCAL_LOG_LEVEL, "[DEBUG]")
#define LOG_INFO  Logger::GetInstance().GetStream(LogLevel::Info,  LOCAL_LOG_LEVEL, "[INFO] ")
#define LOG_WARN  Logger::GetInstance().GetStream(LogLevel::Warning, LOCAL_LOG_LEVEL, "[WARN] ")
#define LOG_ERROR Logger::GetInstance().GetStream(LogLevel::Error, LOCAL_LOG_LEVEL, "[ERROR]")






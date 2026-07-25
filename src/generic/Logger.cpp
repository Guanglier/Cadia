#include "Logger.h"

// --- TeeBuf Implementation ---
TeeBuf::TeeBuf(std::streambuf* sb1, std::streambuf* sb2) : sb1_(sb1), sb2_(sb2) {}

int TeeBuf::overflow(int c) {
    if (c == EOF) return EOF;
    if (sb1_ && sb1_->sputc(traits_type::to_char_type(c)) == EOF) return EOF;
    if (sb2_ && sb2_->sputc(traits_type::to_char_type(c)) == EOF) return EOF;
    return c;
}

int TeeBuf::sync() {
    int res1 = sb1_ ? sb1_->pubsync() : 0;
    int res2 = sb2_ ? sb2_->pubsync() : 0;
    return (res1 == 0 && res2 == 0) ? 0 : -1;
}


// --- Logger Implementation ---
Logger::Logger() : currentLevel_(LogLevel::Debug) {
    // Flux par défaut : uniquement la console (std::cout)
    logStream_ = std::make_unique<std::ostream>(std::cout.rdbuf());
    
    // Flux poubelle pour les niveaux ignorés
    nullStream_ = std::make_unique<std::ostream>(nullptr);
}

Logger::~Logger() = default;

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

void Logger::SetLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentLevel_ = level;
}

bool Logger::EnableFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (logFile_.is_open()) {
        logFile_.close();
    }

    logFile_.open(filename, std::ios::out | std::ios::app);
    if (!logFile_.is_open()) {
        return false;
    }

    // On crée un buffer double liant std::cout et le fichier log
    teeBuf_ = std::make_unique<TeeBuf>(std::cout.rdbuf(), logFile_.rdbuf());
    logStream_ = std::make_unique<std::ostream>(teeBuf_.get());

    return true;
}

std::ostream& Logger::GetStream(LogLevel level, const char* prefix) {
    if (level < currentLevel_) {
        return *nullStream_; // Retourne un flux mort si le niveau est trop bas
    }

    // On injecte le préfixe au début de la ligne
    // Note: Pour être parfaitement thread-safe sur des lignes entières avec des << multiples, 
    // tu peux aussi protéger par un verrou, mais pour un usage classique, le flux suffit.
    *logStream_ << prefix << " ";
    return *logStream_;
}

std::mutex& Logger::GetMutex() {
    return mutex_;
}



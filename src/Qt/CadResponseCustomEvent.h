#include <QEvent>

// Événement personnalisé pour encapsuler ta structure C++ pure
class CadResponseCustomEvent : public QEvent {
public:
    // Identifiant unique d'événement propre à ton application (au-delà de QEvent::User)
    static const QEvent::Type EventType = static_cast<QEvent::Type>(QEvent::User + 101);

    explicit CadResponseCustomEvent(const CadResponseEvent& resp)
        : QEvent(EventType), m_response(resp) {}

    const CadResponseEvent& getResponse() const { return m_response; }

private:
    CadResponseEvent m_response;
};





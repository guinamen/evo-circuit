#pragma once
// =============================================================================
// Logger.hpp — Singleton de log para o evo-circuit (spdlog 1.12)
//
// Uso:
//   #include "evo-circuit/core/Logger.hpp"
//
//   // Inicializar uma vez no main() antes de qualquer uso:
//   evo_circuit::Logger::init("evo-circuit", spdlog::level::debug, "logs/run.log");
//
//   // Em qualquer arquivo após o include:
//   EVO_LOG_INFO("Run iniciada: benchmark={} seed={}", plu_file, seed);
//   EVO_LOG_DEBUG("Geração {}: fitness={} ativos={}", gen, fit, n_active);
//   EVO_LOG_TRACE("Nó {}: f={} in=[{},{}] out={}", node, func, a, b, result);
//
// Níveis (do mais ao menos verboso):
//   trace  — avaliação de cada nó (compile-time disabled em Release)
//   debug  — cada geração com fitness e estado do circuito
//   info   — início/fim de run, configuração, resultado
//   warn   — platôs, timeouts, situações inesperadas mas toleráveis
//   error  — falhas de arquivo, inconsistências, exceções capturadas
// =============================================================================

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/pattern_formatter.h>
#include <memory>
#include <string>
#include <vector>

// ── Macros de log (usam o logger "evo-circuit") ───────────────────────────
// TRACE é compilado fora em Release (SPDLOG_ACTIVE_LEVEL >= SPDLOG_LEVEL_DEBUG)
#define EVO_LOG_TRACE(...)  SPDLOG_LOGGER_TRACE(  ::evo_circuit::Logger::get(), __VA_ARGS__)
#define EVO_LOG_DEBUG(...)  SPDLOG_LOGGER_DEBUG(  ::evo_circuit::Logger::get(), __VA_ARGS__)
#define EVO_LOG_INFO(...)   SPDLOG_LOGGER_INFO(   ::evo_circuit::Logger::get(), __VA_ARGS__)
#define EVO_LOG_WARN(...)   SPDLOG_LOGGER_WARN(   ::evo_circuit::Logger::get(), __VA_ARGS__)
#define EVO_LOG_ERROR(...)  SPDLOG_LOGGER_ERROR(  ::evo_circuit::Logger::get(), __VA_ARGS__)

namespace evo_circuit {

class Logger {
public:
    /// Inicializa o logger. Chame uma vez no main() antes de qualquer uso.
    /// @param name      Nome do logger (aparece no output)
    /// @param level     Nível mínimo: trace/debug/info/warn/error/off
    /// @param log_file  Caminho do arquivo de log ("" = somente console)
    /// @param max_mb    Tamanho máximo do arquivo antes de rotacionar (MB)
    /// @param max_files Número de arquivos rotativos mantidos
    static void init(
        const std::string& name      = "evo-circuit",
        spdlog::level::level_enum level = spdlog::level::info,
        const std::string& log_file  = "",
        size_t max_mb    = 10,
        size_t max_files = 3)
    {
        std::vector<spdlog::sink_ptr> sinks;

        // Console colorido: DEBUG+ em stderr
        auto console = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        console->set_level(level);
        console->set_pattern("%^[%H:%M:%S.%e] [%l]%$ %v");
        sinks.push_back(console);

        // Arquivo rotativo (se path informado)
        if (!log_file.empty()) {
            // Criar diretório pai se necessário
            std::string dir = log_file.substr(0, log_file.find_last_of("/\\"));
            if (!dir.empty() && dir != log_file) {
                std::filesystem::create_directories(dir);
            }
            auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_file, max_mb * 1024 * 1024, max_files);
            file->set_level(spdlog::level::trace); // arquivo captura tudo
            file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");
            sinks.push_back(file);
        }

        auto logger = std::make_shared<spdlog::logger>(
            name, sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace); // o sink filtra individualmente
        logger->flush_on(spdlog::level::warn);   // flush imediato em warn/error

        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);

        logger_ = logger;
        EVO_LOG_INFO("Logger inicializado: nível={} arquivo={}",
            spdlog::level::to_string_view(level),
            log_file.empty() ? "somente console" : log_file);
    }

    /// Retorna o logger. init() deve ter sido chamado antes.
    static std::shared_ptr<spdlog::logger> get() {
        if (!logger_) {
            // Fallback: logger padrão de console se init() não foi chamado
            logger_ = spdlog::stdout_color_mt("evo-circuit-default");
            logger_->set_level(spdlog::level::info);
        }
        return logger_;
    }

    /// Converte string para level_enum.
    static spdlog::level::level_enum level_from_string(const std::string& s) {
        if (s == "trace") return spdlog::level::trace;
        if (s == "debug") return spdlog::level::debug;
        if (s == "info")  return spdlog::level::info;
        if (s == "warn")  return spdlog::level::warn;
        if (s == "error") return spdlog::level::err;
        if (s == "off")   return spdlog::level::off;
        return spdlog::level::info; // padrão
    }

    /// Flush explícito (útil antes de encerrar o programa).
    static void flush() {
        if (logger_) logger_->flush();
    }

private:
    static inline std::shared_ptr<spdlog::logger> logger_;
};

} // namespace evo_circuit

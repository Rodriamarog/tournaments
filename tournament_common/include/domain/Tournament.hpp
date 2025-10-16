// ===============================================================
// Archivo: Tournament.hpp
// Descripción: Define las clases principales del dominio relacionadas
// con torneos, incluyendo su formato, grupos y partidos.
// Pertenece al namespace 'domain'.
// ===============================================================

#ifndef DOMAIN_TOURNAMENT_HPP
#define DOMAIN_TOURNAMENT_HPP

#include <string>
#include <vector>

#include "domain/Group.hpp"
#include "domain/Match.hpp"

namespace domain {

    // Enumeración que define los tipos posibles de torneo.
    // ROUND_ROBIN → Todos contra todos.
    // NFL → Eliminaroria o formato tipo NFL.
    enum class TournamentType {
        ROUND_ROBIN, 
        NFL
    };

    // ---------------------------------------------------------------
    // Clase: TournamentFormat
    // Propósito: Representa la configuración general del torneo,
    // incluyendo cantidad de grupos, equipos máximos y tipo de torneo.
    // ---------------------------------------------------------------
    class TournamentFormat {
        int numberOfGroups;       // Cantidad de grupos en el torneo
        int maxTeamsPerGroup;     // Número máximo de equipos por grupo
        TournamentType type;      // Tipo de torneo (Round Robin, NFL, etc.)

    public:
        // Constructor con valores predeterminados
        TournamentFormat(
            int numberOfGroups = 1, 
            int maxTeamsPerGroup = 16, 
            TournamentType tournamentType = TournamentType::ROUND_ROBIN
        ) {
            this->numberOfGroups = numberOfGroups;
            this->maxTeamsPerGroup = maxTeamsPerGroup;
            this->type = tournamentType;
        }

        // Métodos de acceso (getters y setters)
        int NumberOfGroups() const {
            return this->numberOfGroups;
        }
        int & NumberOfGroups() {
            return this->numberOfGroups;
        }

        int MaxTeamsPerGroup() const {
            return this->maxTeamsPerGroup;
        }
        int & MaxTeamsPerGroup() {
            return this->maxTeamsPerGroup;
        }

        TournamentType Type() const {
            return this->type;
        }
        TournamentType & Type() {
            return this->type;
        }
    };

    // ---------------------------------------------------------------
    // Clase: Tournament
    // Propósito: Representa un torneo completo, incluyendo su nombre,
    // formato, grupos participantes y partidos programados.
    // ---------------------------------------------------------------
    class Tournament {
        std::string id;                  // Identificador único del torneo
        std::string name;                // Nombre del torneo
        TournamentFormat format;         // Formato (número de grupos, tipo, etc.)
        std::vector<Group> groups;       // Lista de grupos del torneo
        std::vector<Match> matches;      // Lista de partidos del torneo

    public:
        // Constructor que inicializa el nombre y el formato del torneo
        explicit Tournament(
            const std::string &name = "", 
            TournamentFormat format = TournamentFormat()
        ) {
            this->name = name;
            this->format = format;
        }

        // --------- Métodos de acceso (getters y setters) ---------

        [[nodiscard]] std::string Id() const {
            return this->id;
        }
        std::string& Id() {
            return this->id;
        }

        [[nodiscard]] std::string Name() const {
            return this->name;
        }
        std::string& Name() {
            return this->name;
        }

        [[nodiscard]] TournamentFormat Format() const {
            return this->format;
        }
        TournamentFormat & Format() {
            return this->format;
        }

        [[nodiscard]] std::vector<Group> & Groups() {
            return this->groups;
        }

        [[nodiscard]] std::vector<Match> Matches() const {
            return this->matches;
        }
    };
}

#endif // DOMAIN_TOURNAMENT_HPP

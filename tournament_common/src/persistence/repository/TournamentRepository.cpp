//
// Created by tsuny on 9/1/25.
//
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "persistence/repository/TournamentRepository.hpp"
#include "domain/Utilities.hpp"
#include "persistence/configuration/PostgresConnection.hpp"

TournamentRepository::TournamentRepository(std::shared_ptr<IDbConnectionProvider> connection)
    : connectionProvider(std::move(connection)) {
    // Constructor: recibe un proveedor de conexiones (pool, fábrica, etc.)
    // Se guarda en el miembro connectionProvider para usarlo en los métodos.
}

std::shared_ptr<domain::Tournament> TournamentRepository::ReadById(std::string id) {
    // Obtener una conexión del proveedor
    auto pooled = connectionProvider->Connection();
    // Se asume que pooled apunta internamente a un PostgresConnection
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    // Iniciar una transacción de PostgreSQL usando pqxx
    pqxx::work tx(*(connection->connection));
    // Ejecutar una consulta preparada ("select_tournament_by_id") con el parámetro id
    const pqxx::result result = tx.exec(pqxx::prepped{"select_tournament_by_id"}, id);
    // Confirmar la transacción
    tx.commit();

    // Si no se encontró ninguna fila, retornar nullptr para indicar “no existe”
    if (result.empty()) {
        return nullptr;
    }

    // Obtener el campo “document” de la primera fila, que está en JSON (texto)
    nlohmann::json rowTournament = nlohmann::json::parse(result.at(0)["document"].c_str());

    // Crear un objeto domain::Tournament a partir del JSON
    auto tournament = std::make_shared<domain::Tournament>(rowTournament);
    // También asignar el campo “id” del resultado al objeto
    tournament->Id() = result.at(0)["id"].c_str();

    return tournament;
}

std::string TournamentRepository::Create(const domain::Tournament & entity) {
    // Convertir el objeto Tournament a json (serialización)
    const nlohmann::json tournamentDoc = entity;

    // Obtener una conexión para operar
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    // Iniciar transacción
    pqxx::work tx(*(connection->connection));
    // Ejecutar consulta preparada "insert_tournament", con el documento JSON como cadena
    const pqxx::result result = tx.exec(pqxx::prepped{"insert_tournament"}, tournamentDoc.dump());
    // Confirmar la transacción
    tx.commit();

    // Retornar el id generado (se espera que la consulta devuelva el id al insertar)
    return result[0]["id"].c_str();
}

std::string TournamentRepository::Update(const domain::Tournament & entity) {
    // Método no implementado aún
    // - serializar el entity a JSON
    // - ejecutar una consulta preparada tipo “update_tournament” con id y JSON
    // - retornar el id o estado de la operación
    return "id";
}

void TournamentRepository::Delete(std::string id) {
    // Método no implementado aún
    // - ejecutar una consulta preparada “delete_tournament” con ese id
    // - posiblemente retornar éxito/fallo, o lanzar excepción en caso de error
}

std::vector<std::shared_ptr<domain::Tournament>> TournamentRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    // Obtener conexión
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    // Crear transacción
    pqxx::work tx(*(connection->connection));
    // Ejecutar consulta directa (no preparada) para recuperar todos los torneos
    const pqxx::result result{ tx.exec("select id, document from tournaments") };
    tx.commit();

    // Para cada fila del resultado:
    for (auto row : result) {
        // Parsear el JSON del campo "document"
        nlohmann::json rowTournament = nlohmann::json::parse(row["document"].c_str());
        // Crear objeto domain::Tournament desde el JSON
        auto tournament = std::make_shared<domain::Tournament>(rowTournament);
        // Asignar el id del torneo
        tournament->Id() = row["id"].c_str();

        // Añadir al vector resultado
        tournaments.push_back(tournament);
    }

    return tournaments;
}

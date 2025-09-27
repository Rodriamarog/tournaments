-- Tournament System Database Schema - JSONB Compatible with existing C++ code
-- Based on existing db_script.sql but improved for Supabase

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- TEAMS table (compatible with existing TournamentRepository.cpp)
CREATE TABLE TEAMS (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    document JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create unique index for team names (from document JSON)
CREATE UNIQUE INDEX team_unique_name_idx ON TEAMS ((document->>'name'));

-- TOURNAMENTS table (compatible with existing TournamentRepository.cpp)
CREATE TABLE TOURNAMENTS (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    document JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create unique index for tournament names
CREATE UNIQUE INDEX tournament_unique_name_idx ON TOURNAMENTS ((document->>'name'));

-- GROUPS table (fixed reference to TOURNAMENTS instead of TEAMS)
CREATE TABLE GROUPS (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    TOURNAMENT_ID UUID REFERENCES TOURNAMENTS(ID) ON DELETE CASCADE, -- Fixed: was referencing TEAMS
    document JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create unique index for group names within tournament (fixed typo: gruop -> group)
CREATE UNIQUE INDEX tournament_group_unique_name_idx ON GROUPS (tournament_id, (document->>'name'));

-- MATCHES table
CREATE TABLE MATCHES (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    TOURNAMENT_ID UUID REFERENCES TOURNAMENTS(ID) ON DELETE CASCADE,
    document JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Additional performance indexes for JSONB queries
CREATE INDEX idx_teams_document_gin ON TEAMS USING gin (document);
CREATE INDEX idx_tournaments_document_gin ON TOURNAMENTS USING gin (document);
CREATE INDEX idx_groups_document_gin ON GROUPS USING gin (document);
CREATE INDEX idx_matches_document_gin ON MATCHES USING gin (document);

-- Indexes for common JSONB queries
CREATE INDEX idx_tournaments_status ON TOURNAMENTS ((document->>'status'));
CREATE INDEX idx_tournaments_type ON TOURNAMENTS ((document->>'type'));
CREATE INDEX idx_matches_status ON MATCHES ((document->>'status'));
CREATE INDEX idx_matches_home_team ON MATCHES ((document->>'homeTeamId'));
CREATE INDEX idx_matches_away_team ON MATCHES ((document->>'awayTeamId'));

-- Function to update created_at to current timestamp (useful for updates)
CREATE OR REPLACE FUNCTION update_timestamp()
RETURNS TRIGGER AS $$
BEGIN
    NEW.created_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Views for easier querying (optional, for reporting)
CREATE VIEW tournament_summary AS
SELECT
    id,
    document->>'name' as tournament_name,
    document->>'status' as status,
    document->>'type' as tournament_type,
    created_at
FROM TOURNAMENTS;

CREATE VIEW team_summary AS
SELECT
    id,
    document->>'name' as team_name,
    created_at
FROM TEAMS;
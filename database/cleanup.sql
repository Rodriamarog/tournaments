-- Script to remove unnecessary views and functions added to Supabase
-- Run this in Supabase SQL Editor to clean up

-- Drop the views that were not in the original script
DROP VIEW IF EXISTS tournament_summary;
DROP VIEW IF EXISTS team_summary;

-- Drop the function that was not in the original script
DROP FUNCTION IF EXISTS update_timestamp();
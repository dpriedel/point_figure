-- create table to store PF_Chart data

CREATE SCHEMA IF NOT EXISTS test_point_and_figure AUTHORIZATION data_updater_pg;

DROP TYPE IF EXISTS DIRECTION CASCADE;
CREATE TYPE direction AS ENUM ('e_unknown', 'e_up', 'e_down');

DROP TYPE IF EXISTS BOXTYPE CASCADE;
CREATE TYPE boxtype AS ENUM ('e_fractional', 'e_integral');

DROP TYPE IF EXISTS BOXSCALE CASCADE;
CREATE TYPE boxscale AS ENUM ('e_linear', 'e_percent');

DROP TABLE IF EXISTS test_point_and_figure.pf_charts CASCADE;

CREATE TABLE test_point_and_figure.pf_charts
(
    chart_id BIGINT GENERATED ALWAYS AS IDENTITY UNIQUE,
    symbol TEXT NOT NULL,
    fname_box_size NUMERIC(8, 4),
    reversal_boxes INTEGER,
    box_type BOXTYPE,
    box_scale BOXSCALE,
    file_name TEXT NOT NULL,
    first_date TIMESTAMP NOT NULL,
    last_change_date TIMESTAMP NOT NULL,
    last_checked_date TIMESTAMP NOT NULL,
    current_direction DIRECTION NOT NULL,
    chart_data JSONB NOT NULL,
    cvs_graphics_data TEXT DEFAULT NULL,
    UNIQUE(file_name),
    PRIMARY KEY(file_name)
);

ALTER TABLE test_point_and_figure.pf_charts OWNER TO data_updater_pg;

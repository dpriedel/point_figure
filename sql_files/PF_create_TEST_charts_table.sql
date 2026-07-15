-- create table to store PF_Chart data

CREATE SCHEMA IF NOT EXISTS test_point_and_figure;
ALTER SCHEMA test_point_and_figure OWNER TO data_updater_pg;

DROP TYPE IF EXISTS TEST_DIRECTION CASCADE;
CREATE TYPE test_direction AS ENUM ('e_unknown', 'e_up', 'e_down');
ALTER TYPE test_direction OWNER TO data_updater_pg;

DROP TYPE IF EXISTS TEST_BOXTYPE CASCADE;
CREATE TYPE test_boxtype AS ENUM ('e_fractional', 'e_integral');
ALTER TYPE test_boxtype OWNER TO data_updater_pg;

DROP TYPE IF EXISTS TEST_BOXSCALE CASCADE;
CREATE TYPE test_boxscale AS ENUM ('e_linear', 'e_percent');
ALTER TYPE test_boxscale OWNER TO data_updater_pg;

DROP TYPE IF EXISTS TEST_SIGNALTYPE CASCADE;
CREATE TYPE test_signaltype AS ENUM (
    'e_unknown',
    'e_double_top_buy',
    'e_double_bottom_sell',
    'e_triple_top_buy',
    'e_triple_bottom_sell',
    'e_bullish_tt_buy',
    'e_bearish_tb_sell',
    'e_catapult_buy',
    'e_catapult_sell',
    'e_ttop_catapult_buy',
    'e_tbottom_catapult_sell'
);
ALTER TYPE test_signaltype OWNER TO data_updater_pg;

DROP TABLE IF EXISTS test_point_and_figure.pf_charts CASCADE;

CREATE TABLE test_point_and_figure.pf_charts
(
    chart_id BIGINT GENERATED ALWAYS AS IDENTITY UNIQUE,
    symbol TEXT NOT NULL,
    fname_box_size NUMERIC(8, 4),
    chart_box_size NUMERIC(8, 4),
    reversal_boxes INTEGER,
    box_type TEST_BOXTYPE,
    box_scale TEST_BOXSCALE,
    file_name TEXT NOT NULL,
    first_date TIMESTAMP WITH TIME ZONE NOT NULL,
    last_change_date TIMESTAMP WITH TIME ZONE NOT NULL,
    last_checked_date TIMESTAMP WITH TIME ZONE NOT NULL,
    current_direction TEST_DIRECTION NOT NULL,
    current_signal TEST_SIGNALTYPE NOT NULL,
    chart_data JSONB NOT NULL,
    cvs_graphics_data TEXT DEFAULT NULL,
    UNIQUE (file_name),
    PRIMARY KEY (file_name)
);

ALTER TABLE test_point_and_figure.pf_charts OWNER TO data_updater_pg;

DROP TABLE IF EXISTS new_stock_data.gold;

CREATE TABLE new_stock_data.gold ( -- noqa: disable=L008
    symbol TEXT NOT NULL,
    date DATE NOT NULL,
    open_p NUMERIC(12,4) DEFAULT '0',
    high NUMERIC(12,4) DEFAULT '0',
    low NUMERIC(12,4) DEFAULT '0',
    close_p NUMERIC(12,4) DEFAULT '0',
    volume BIGINT DEFAULT '0',
    close_up_or_down SMALLINT, -- noqa: enable=L008

    PRIMARY KEY (symbol, date)
);
ALTER TABLE new_stock_data.gold OWNER TO data_updater_pg;

DROP TABLE IF EXISTS new_stock_data.bdi;

CREATE TABLE new_stock_data.bdi ( -- noqa: disable=L008
    date DATE NOT NULL,
    open_p NUMERIC(12,4) DEFAULT '0',
    high NUMERIC(12,4) DEFAULT '0',
    low NUMERIC(12,4) DEFAULT '0',
    close_p NUMERIC(12,4) DEFAULT '0',
    volume BIGINT DEFAULT '0',
    close_up_or_down SMALLINT, -- noqa: enable=L008

    PRIMARY KEY (date)
);
ALTER TABLE new_stock_data.bdi OWNER TO data_updater_pg;

DROP TRIGGER IF EXISTS new_gold_insert_trg ON new_stock_data.gold;

CREATE TRIGGER new_gold_insert_trg -- noqa: disable=L003
    BEFORE INSERT ON new_stock_data.gold
    FOR EACH ROW
        EXECUTE PROCEDURE public.new_compute_close_up_or_down();

DROP TRIGGER IF EXISTS new_gold_update_trg ON new_stock_data.gold;

CREATE TRIGGER new_gold_update_trg
    BEFORE UPDATE ON new_stock_data.gold
    FOR EACH ROW
        EXECUTE PROCEDURE public.new_compute_close_up_or_down();

-- DROP TRIGGER IF EXISTS new_bdi_insert_trg ON new_stock_data.bdi;
--
-- CREATE TRIGGER new_bdi_insert_trg -- noqa: disable=L003
--     BEFORE INSERT ON new_stock_data.bdi
--     FOR EACH ROW
--         EXECUTE PROCEDURE public.new_compute_close_up_or_down();
--
-- DROP TRIGGER IF EXISTS new_bdi_update_trg ON new_stock_data.bdi;
--
-- CREATE TRIGGER new_bdi_update_trg
--     BEFORE UPDATE ON new_stock_data.bdi
--     FOR EACH ROW
--         EXECUTE PROCEDURE public.new_compute_close_up_or_down();

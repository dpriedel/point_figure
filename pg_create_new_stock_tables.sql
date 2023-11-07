-- DROP TABLE IF EXISTS public.new_holidays;
--
-- CREATE TABLE public.new_holidays (
--  date DATE,
--  name TEXT,
--
--  PRIMARY KEY (date)
-- );
--
-- ALTER TABLE public.new_holidays OWNER TO data_updater_pg;

DROP TABLE IF EXISTS new_stock_data.current_data;

CREATE TABLE new_stock_data.current_data ( -- noqa: disable=L008
    symbol TEXT NOT NULL,
    date DATE NOT NULL,
    open_p NUMERIC(12,4) NULL DEFAULT '0',
    high NUMERIC(12,4) NULL DEFAULT '0',
    low NUMERIC(12,4) NULL DEFAULT '0',
    close_p NUMERIC(12,4) NULL DEFAULT '0',
    volume BIGINT NULL DEFAULT '0',
    adjopen NUMERIC(12,4) NULL DEFAULT '-1',
    adjhigh NUMERIC(12,4) NULL DEFAULT '-1',
    adjlow NUMERIC(12,4) NULL DEFAULT '-1',
    adjclose NUMERIC(12,4) NULL DEFAULT '-1',
    adjvolume BIGINT NULL DEFAULT '-1',
    alternate_close NUMERIC(12,4) NULL DEFAULT '0',
    close_up_or_down SMALLINT, -- noqa: enable=L008

    PRIMARY KEY (symbol, date)
);
ALTER TABLE new_stock_data.current_data OWNER TO data_updater_pg;

DROP TABLE IF EXISTS new_stock_data.index_history;

CREATE TABLE new_stock_data.index_history ( -- noqa: disable=L008
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
ALTER TABLE new_stock_data.index_history OWNER TO data_updater_pg;

DROP TABLE IF EXISTS new_stock_data.names_and_symbols;

CREATE TABLE new_stock_data.names_and_symbols ( -- noqa: disable=L008
    exchange TEXT NOT NULL,
    symbol TEXT NOT NULL,
    name TEXT DEFAULT 'Unknown', -- noqa: enable=L008

    PRIMARY KEY (exchange, symbol)
);
ALTER TABLE new_stock_data.names_and_symbols OWNER TO data_updater_pg;

DROP FUNCTION IF EXISTS public.new_compute_close_up_or_down() CASCADE;

CREATE FUNCTION public.new_compute_close_up_or_down()
RETURNS TRIGGER
LANGUAGE plpgsql
AS
$$
BEGIN
    IF NEW.open_p > NEW.close_p THEN
        NEW.close_up_or_down := -1; -- closed down
    ELSIF NEW.close_p > NEW.open_p THEN
        NEW.close_up_or_down := 1; -- closed up
    ELSE
        NEW.close_up_or_down := 0; -- no change
    END IF;
    RETURN NEW;
END;
$$;

ALTER FUNCTION public.new_compute_close_up_or_down() OWNER TO data_updater_pg;

DROP FUNCTION IF EXISTS public.new_compute_adjclose_up_or_down() CASCADE;

CREATE FUNCTION public.new_compute_adjclose_up_or_down()
RETURNS TRIGGER
LANGUAGE plpgsql
AS
$$
BEGIN
    IF NEW.adjopen = -1 THEN
        NEW.adjopen = NEW.open_p;
    END IF;
    IF NEW.adjhigh = -1 THEN
        NEW.adjhigh = NEW.high;
    END IF;
    IF NEW.adjlow = -1 THEN
        NEW.adjlow = NEW.low;
    END IF;
    IF NEW.adjclose = -1 THEN
        NEW.adjclose = NEW.close_p;
    END IF;
    IF NEW.adjvolume = -1 THEN
        NEW.adjvolume = NEW.volume;
    END IF;

    IF NEW.adjopen > NEW.adjclose THEN
        NEW.close_up_or_down := -1; -- closed down
    ELSIF NEW.adjclose > NEW.adjopen THEN
        NEW.close_up_or_down := 1; -- close_p up
    ELSE
        NEW.close_up_or_down := 0; -- no change
    END IF;
    RETURN NEW;
END;
$$;

ALTER FUNCTION public.new_compute_adjclose_up_or_down() OWNER TO data_updater_pg;

DROP TRIGGER IF EXISTS new_current_data_insert_trg ON new_stock_data.current_data;

CREATE TRIGGER new_current_data_insert_trg -- noqa: disable=L003
    BEFORE INSERT ON new_stock_data.current_data
    FOR EACH ROW
        EXECUTE PROCEDURE public.new_compute_adjclose_up_or_down();

DROP TRIGGER IF EXISTS new_current_data_update_trg ON new_stock_data.current_data;

CREATE TRIGGER new_current_data_update_trg
    BEFORE UPDATE ON new_stock_data.current_data
    FOR EACH ROW
        EXECUTE PROCEDURE public.new_compute_adjclose_up_or_down();

DROP TRIGGER IF EXISTS new_index_history_insert_trg ON new_stock_data.index_history;

CREATE TRIGGER new_index_history_insert_trg
    BEFORE INSERT ON new_stock_data.index_history
    FOR EACH ROW
        EXECUTE PROCEDURE public.new_compute_close_up_or_down();

DROP TRIGGER IF EXISTS new_index_history_update_trg ON new_stock_data.index_history;

CREATE TRIGGER new_index_history_update_trg
    BEFORE UPDATE ON new_stock_data.index_history
    FOR EACH ROW
       EXECUTE PROCEDURE public.new_compute_close_up_or_down();
    -- noqa: enable=L003

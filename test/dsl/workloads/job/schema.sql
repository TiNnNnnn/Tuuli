CREATE TABLE aka_name (
  id INT NOT NULL PRIMARY KEY,
  person_id INT NOT NULL,
  name TEXT NOT NULL,
  imdb_index VARCHAR(12),
  name_pcode_cf VARCHAR(5),
  name_pcode_nf VARCHAR(5),
  surname_pcode VARCHAR(5),
  md5sum VARCHAR(32)
);

CREATE TABLE aka_title (
  id INT NOT NULL PRIMARY KEY,
  movie_id INT NOT NULL,
  title TEXT NOT NULL,
  imdb_index VARCHAR(12),
  kind_id INT NOT NULL,
  production_year INT,
  phonetic_code VARCHAR(5),
  episode_of_id INT,
  season_nr INT,
  episode_nr INT,
  note TEXT,
  md5sum VARCHAR(32)
);

CREATE TABLE cast_info (
  id INT NOT NULL PRIMARY KEY,
  person_id INT NOT NULL,
  movie_id INT NOT NULL,
  person_role_id INT,
  note TEXT,
  nr_order INT,
  role_id INT NOT NULL
);

CREATE TABLE char_name (
  id INT NOT NULL PRIMARY KEY,
  name TEXT NOT NULL,
  imdb_index VARCHAR(12),
  imdb_id INT,
  name_pcode_nf VARCHAR(5),
  surname_pcode VARCHAR(5),
  md5sum VARCHAR(32)
);

CREATE TABLE comp_cast_type (
  id INT NOT NULL PRIMARY KEY,
  kind VARCHAR(32) NOT NULL
);

CREATE TABLE company_name (
  id INT NOT NULL PRIMARY KEY,
  name TEXT NOT NULL,
  country_code VARCHAR(255),
  imdb_id INT,
  name_pcode_nf VARCHAR(5),
  name_pcode_sf VARCHAR(5),
  md5sum VARCHAR(32)
);

CREATE TABLE company_type (
  id INT NOT NULL PRIMARY KEY,
  kind VARCHAR(32) NOT NULL
);

CREATE TABLE complete_cast (
  id INT NOT NULL PRIMARY KEY,
  movie_id INT,
  subject_id INT NOT NULL,
  status_id INT NOT NULL
);

CREATE TABLE info_type (
  id INT NOT NULL PRIMARY KEY,
  info VARCHAR(32) NOT NULL
);

CREATE TABLE keyword (
  id INT NOT NULL PRIMARY KEY,
  keyword TEXT NOT NULL,
  phonetic_code VARCHAR(5)
);

CREATE TABLE kind_type (
  id INT NOT NULL PRIMARY KEY,
  kind VARCHAR(15) NOT NULL
);

CREATE TABLE link_type (
  id INT NOT NULL PRIMARY KEY,
  link VARCHAR(32) NOT NULL
);

CREATE TABLE movie_companies (
  id INT NOT NULL PRIMARY KEY,
  movie_id INT NOT NULL,
  company_id INT NOT NULL,
  company_type_id INT NOT NULL,
  note TEXT
);

CREATE TABLE movie_info (
  id INT NOT NULL PRIMARY KEY,
  movie_id INT NOT NULL,
  info_type_id INT NOT NULL,
  info TEXT NOT NULL,
  note TEXT
);

CREATE TABLE movie_info_idx (
  id INT NOT NULL PRIMARY KEY,
  movie_id INT NOT NULL,
  info_type_id INT NOT NULL,
  info TEXT NOT NULL,
  note TEXT
);

CREATE TABLE movie_keyword (
  id INT NOT NULL PRIMARY KEY,
  movie_id INT NOT NULL,
  keyword_id INT NOT NULL
);

CREATE TABLE movie_link (
  id INT NOT NULL PRIMARY KEY,
  movie_id INT NOT NULL,
  linked_movie_id INT NOT NULL,
  link_type_id INT NOT NULL
);

CREATE TABLE name (
  id INT NOT NULL PRIMARY KEY,
  name TEXT NOT NULL,
  imdb_index VARCHAR(12),
  imdb_id INT,
  gender VARCHAR(1),
  name_pcode_cf VARCHAR(5),
  name_pcode_nf VARCHAR(5),
  surname_pcode VARCHAR(5),
  md5sum VARCHAR(32)
);

CREATE TABLE person_info (
  id INT NOT NULL PRIMARY KEY,
  person_id INT NOT NULL,
  info_type_id INT NOT NULL,
  info TEXT NOT NULL,
  note TEXT
);

CREATE TABLE role_type (
  id INT NOT NULL PRIMARY KEY,
  role VARCHAR(32) NOT NULL
);

CREATE TABLE title (
  id INT NOT NULL PRIMARY KEY,
  title TEXT NOT NULL,
  imdb_index VARCHAR(12),
  kind_id INT NOT NULL,
  production_year INT,
  imdb_id INT,
  phonetic_code VARCHAR(5),
  episode_of_id INT,
  season_nr INT,
  episode_nr INT,
  series_years VARCHAR(49),
  md5sum VARCHAR(32)
);

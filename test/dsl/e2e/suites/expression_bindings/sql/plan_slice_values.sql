SELECT * FROM (VALUES
  ('-9223372036854775808'::bigint, NULL::integer, true),
  ('-9223372036854775808'::bigint, NULL::integer, true),
  (7::bigint, 3::integer, NULL::boolean)
) AS v(a, b, c);

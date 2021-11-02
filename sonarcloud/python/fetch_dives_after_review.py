from ansitable import ANSITable

# Split line into tokens without trailing whitespace or Markdown emphasis
def __clean_line(line) -> list[str]:
    return [token.strip().replace("**", "") for token in line.split("|") if token]


# Highlight warnings if they exist
def __format_warning(warning) -> str:
    if warning == "None":
        return "None"
    elif warning == "TBA":
        return "(TBA)" # Highlight in white
    else:
        return f"[{warning}]" # Highlight in red


# Populate a table with prealigned columns
def __insert_columns(table, header_text) -> ANSITable:
    columns_text = __clean_line(header_text)[1:]
    for name in columns_text:
        table.addColumn(name, headalign="<", colalign="<")
    return table


# Strip the outer border from a table
def __remove_table_border(table) -> str:
    lines = table.__str__().strip().split("\n")
    for idx, line in enumerate(lines):
        lines[idx] = line[1:-1]
    return "\n".join(lines[1:-1])


# From a markdown table, generate one using unicode box-drawing characters
def fetch_dives(post_text) -> dict:
    raw_text = post_text.split("\n")[4:19]

    dives = {
        "dd": {"place": "", "table": ""},
        "edd": {"place": "", "table": ""}
    }

    for dive_type, dive_raw in {"dd": raw_text[:7], "edd": raw_text[8:]}.items():
        table = __insert_columns(
            ANSITable(border = "thin", colsep = 2, color = False),
            __clean_line(dive_raw[2])[1:],
        )

        # Insert rows into table for each stage of the dive
        for row in dive_raw[4:]:
            tokens = __clean_line(row)

            primary_objective = tokens[0]
            secondary_objective = tokens[1]
            anomaly = __format_warning(tokens[2])
            warning = __format_warning(tokens[3])
            table.row(primary_objective, secondary_objective, anomaly, warning)

        dives[dive_type]["place"] = __clean_line(dive_raw[0])[2]
        dives[dive_type]["table"] = __remove_table_border(table)

    return dives


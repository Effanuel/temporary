from ansitable import ANSITable


def _clean_line(line):
	return [
		# Return every token
		token.strip()
		# delimited by `|`
		for token in line.split("|")
		# if it is not empty
		if token
	]

def _format_warning(w):
	# No warning, no special highlighting
	if w == "None":
		return "{0}".format(w)
	# No warning known yet, highlight in white
	elif w == "TBA":
		return "({0})".format(w)
	# Warning found, highlight in red
	else:
		return "[{0}]".format(w)


def fetch_dives(selftext):
	raw_text = selftext.split("\n")[4:19]

	dives = {
		"dd": {
			"place": "",
			"table": ""
		},
		"edd": {
			"place": "",
			"table": ""
		}
	}

	for dive_type, dive_raw in {"dd": raw_text[:7], "edd": raw_text[8:]}.items():
		# Setup columns for table
		table = ANSITable(
			*(_clean_line(dive_raw[2])[1:]),
			border="thin",
			colsep=2,
			color=False
		)
		for col in table.columns:
			col.headalign = "<"
			col.colalign = "<"

		# Add rows to table
		for row in dive_raw[4:]:
			line = _clean_line(row)[1:]
			line[2] = _format_warning(line[2])
			line[3] = _format_warning(line[3])

			table.row(*line)

		# Maintain thick outer border while adding thin inner border
		table_output = table.__str__().strip().split("\n")

		# Replace whacky symbol I don't know why it's there!
		# for idx, line in enumerate(table_output):
		# 	table_output[idx] = line[1:-1].replace("[0m", "")
		for idx, line in enumerate(table_output):
			table_output[idx] = line[1:-1]
		
		dives[dive_type]["place"] = _clean_line(dive_raw[0])[2].replace("**", "")
		dives[dive_type]["table"] = "\n".join(table_output[1:-1])

	return dives

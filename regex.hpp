#ifndef REGEX_HPP
#define REGEX_HPP

#include <stdio.h>
#include <string.h>
#include <pcre2.h>
#include <cassert>

#ifndef MAX_MATCHES
#define MAX_MATCHES 20
#endif

#define _MAX_MATCHES (MAX_MATCHES)

struct RegexMatch
{
	int m_iSubStringCount;
	int *m_offsetVector = nullptr;
};

class Regex
{
public:
	Regex() = delete;

	~Regex()
	{
		Clear();
	}

	// Compile a regular expression.
	//
	// @param pattern       The regular expression pattern.
	// @param cflags        flags for the regular expression.
	Regex(const char *pattern, size_t cflags = 0)
	{
		Clear();

		int errorNumber;
		PCRE2_SIZE errorOffset;
		m_PcreCode = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, cflags, &errorNumber, &errorOffset, nullptr);
		if (!m_PcreCode)
		{
			PCRE2_UCHAR buffer[256];
 			pcre2_get_error_message(errorNumber, buffer, sizeof(buffer));
  			printf("PCRE2 compilation failed at offset %d: %s\n", (int)errorOffset, buffer);
			assert(false);
		}
	}

	// Matches a string against a pre-compiled regular expression pattern.
	//
	// @param str           The string to check.
	// @param offset        Offset in the string to start searching from. MatchOffset returns the offset of the match.
	// @param mflags        flags for the regular expression.
	// @param errcode       Error code, if applicable.
	// @return              Number of captures found or -1 on failure.
	//
	// @note Use the regex handle passed to this function to extract
	//       matches with GetSubString().
	int Match(const char *str, size_t offset = 0, size_t mflags = 0, int *errcode = nullptr)
	{
		ClearMatch();
		m_String = strdup(str);

		pcre2_match_data *pMatchData = pcre2_match_data_create_from_pattern(m_PcreCode, nullptr);
		int rc = pcre2_match(m_PcreCode, (PCRE2_SPTR)m_String, strlen(m_String), offset, mflags, pMatchData, nullptr);
		if (rc <= 0)
		{
			ClearMatch();

			if (rc == PCRE2_ERROR_NOMATCH)
				return 0;

			if (errcode)
				*errcode = rc;

			return -1;
		}

		PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(pMatchData);
		m_Matches[0].m_offsetVector = new int[rc*2];
		for (int i = 0; i < rc; i++)
		{
			m_Matches[0].m_offsetVector[i*2] = ovector[i*2];
			m_Matches[0].m_offsetVector[i*2+1] = ovector[i*2+1];
		}

		m_Matches[0].m_iSubStringCount = rc;
		m_iMatchCount = 1;

		pcre2_match_data_free(pMatchData);
		return rc;
	}


	// Gets all matches from a string against a pre-compiled regular expression pattern.
	//
	// @param str           The string to check.
	// @param mflags        flags for the regular expression.
	// @param errcode       Error code, if applicable.
	// @return              Number of matches found or -1 on failure.
	//
	// @note Use GetSubString() and loop from 0 -> totalmatches - 1.
	int MatchAll(const char *str, size_t mflags = 0, int *errcode = nullptr)
	{
		ClearMatch();
		m_String = strdup(str);
		size_t strLen = strlen(m_String);
		size_t offset = 0;
		int matches = 0;
		int rc = 0;

		while (matches < _MAX_MATCHES && offset < strLen)
		{
			pcre2_match_data *pMatchData = pcre2_match_data_create_from_pattern(m_PcreCode, nullptr);
			rc = pcre2_match(m_PcreCode, (PCRE2_SPTR)m_String, strLen, offset, mflags, pMatchData, nullptr);
			if (rc <= 0)
			{
				pcre2_match_data_free(pMatchData);
				break;
			}

			PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(pMatchData);
			m_Matches[matches].m_offsetVector = new int[rc*2];
			for (int i = 0; i < rc; i++)
			{
				m_Matches[matches].m_offsetVector[i*2] = ovector[i*2];
				m_Matches[matches].m_offsetVector[i*2+1] = ovector[i*2+1];
			}

			//memcpy(m_Matches[matches].m_offsetVector, ovector, sizeof(int)*rc*2);

			offset = m_Matches[matches].m_offsetVector[1];
			m_Matches[matches].m_iSubStringCount = rc;
			matches++;
			pcre2_match_data_free(pMatchData);
		}

		if (rc < PCRE2_ERROR_NOMATCH || rc == 0)
		{
			ClearMatch();
			if (errcode)
				*errcode = rc;
			return -1;
		}

		if (rc == PCRE2_ERROR_NOMATCH && matches == 0)
		{
			ClearMatch();
			return 0;
		}

		m_iMatchCount = matches;
		return matches;
	}

	// Returns a matched substring from a regex handle.
	//
	// Substring ids start at 0 and end at captures-1, where captures is the
	// number returned by Regex.Match or Regex.CaptureCount.
	//
	// @param strid         The index of the expression to get - starts at 0, and ends at captures - 1.
	// @param buffer        The buffer to set to the matching substring.
	// @param maxlen        The maximum string length of the buffer.
	// @param match         Match to get the captures for - starts at 0, and ends at MatchCount() -1
	// @return              Number of bytes written to the buffer.
	//
	// @note strid = 0 is the full captured string, anything else is the capture group index.
	//       if Regex.Match is used match can only be 0
	int GetSubString(int strid, char buffer[], int maxlen, int match = 0)
	{
		if (match >= m_iMatchCount || strid >= m_Matches[match].m_iSubStringCount)
		{
			printf("Invalid match index or str_id passed.\n");
			return 0;
		}

		const char *substr = m_String + m_Matches[match].m_offsetVector[2 * strid];
		int substrLen = m_Matches[match].m_offsetVector[2 * strid + 1] - m_Matches[match].m_offsetVector[2 * strid];

		int i = 0;
		for (i = 0; i < substrLen; i++)
		{
			if (i >= maxlen)
				break;
			buffer[i] = substr[i];
		}

		buffer[i] = '\0';
		return substrLen;
	}

	// Returns number of matches
	//
	// When using Match this is always 1 or 0 (unless an error occured)
	// @return              Total number of matches found.
	int MatchCount()
	{
		return m_iMatchCount;
	}

	// Returns number of captures for a match
	//
	// @param match         Match to get the number of captures for. Match starts at 0, and ends at MatchCount() -1
	// @return              Number of captures in the match.
	//
	// @note Use GetSubString() and loop from 1 -> captures -1 for str_id to get all captures
	int CaptureCount(int match = 0)
	{
		if (match >= m_iMatchCount || match < 0)
		{
			printf("Invalid match index passed.\n");
			return -1;
		}
		return m_Matches[match].m_iSubStringCount;
	}


	// Returns the string offset of a match.
	//
	// @param match         Match to get the offset of. Match starts at 0, and ends at MatchCount() -1
	// @param pos           0=start, 1=ends.
	// @return              Offset of the match in the string.
	int MatchOffset(int match = 0, int pos = 1)
	{
		return m_Matches[match].m_offsetVector[pos];
	}


public:
	int m_iMatchCount;
	RegexMatch m_Matches[_MAX_MATCHES];

private:
	void Clear()
	{
		if (m_PcreCode)
			pcre2_code_free(m_PcreCode);
		if (m_String)
			free(m_String);
		m_PcreCode = nullptr;
		m_String = nullptr;
		m_iMatchCount = 0;

		for (int i = 0; i < _MAX_MATCHES; i++)
		{
			m_Matches[i].m_iSubStringCount = 0;
			delete[] m_Matches[i].m_offsetVector;
		}
	}

	void ClearMatch()
	{
		if (m_String)
			free(m_String);
		m_String = nullptr;
		m_iMatchCount = 0;

		for (int i = 0; i < _MAX_MATCHES; i++)
		{
			m_Matches[i].m_iSubStringCount = 0;
			delete[] m_Matches[i].m_offsetVector;
		}
	}

	pcre2_code *m_PcreCode = nullptr;
	char *m_String = nullptr;
};

#endif /* REGEX_HPP */

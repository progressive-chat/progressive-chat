// SPDX-License-Identifier: AGPL-3.0-only
// One thing an answer cites, in the plain shape a module may see. Agora's own
// citations::CitationRecord is what it mirrors (ports/agora-core citations);
// the ported header stays on the classic side, and what crosses is the parts a
// screen shows: what it came from, what it is called, and where to read it.
#ifndef PCHAT_PROVIDER_CITATION_H
#define PCHAT_PROVIDER_CITATION_H

#include <string>

namespace pchat::provider {

// One source an answer leaned on, said the way Agora's CitationRecord says it:
// where it came from, which of the wire's providers named it, what kind of
// thing it is, what it is called, where to read it, and what of it was quoted.
// Empty where the wire named none.
struct citation {
  std::string source_id;
  std::string provider;
  std::string kind;
  std::string title;
  std::string url;
  std::string excerpt;
};

}  // namespace pchat::provider

#endif  // PCHAT_PROVIDER_CITATION_H

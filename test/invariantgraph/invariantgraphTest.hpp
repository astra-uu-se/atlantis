#pragma once

#define FZN_VALUE(value) std::make_shared<fznparser::ValueLiteral>(value)
#define FZN_SEARCH_VARIABLE(name, lowerBound, upperBound) \
  std::make_shared<fznparser::SearchVariable>(            \
      name, fznparser::AnnotationCollection(),            \
      std::make_unique<fznparser::IntervalDomain>(lowerBound, upperBound))

#define FZN_VECTOR_CONSTRAINT_ARG(...) \
  std::vector<std::shared_ptr<fznparser::Literal>> { __VA_ARGS__ }

#define FZN_NO_ANNOTATIONS fznparser::AnnotationCollection()
#define FZN_DEFINES_VAR_ANNOTATION(name, variable) \
  fznparser::MutableAnnotationCollection name;     \
  (name).add<fznparser::DefinesVarAnnotation>(variable)

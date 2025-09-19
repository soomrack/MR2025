CC := g++

main_O_SET := src_main # список зависимостей для main 
BUILD_DIR := build/
SRC_DIR := src/


TARGET := main



# правило для компиляции
define run_tmpl

$(1).run: $(addsuffix .o,$($(1)_O_SET))
	$$(eval OBJS := $$(addprefix $(BUILD_DIR),$$^))
	# создание .run
	$(CC) $$(OBJS) -o $$@

endef

all: $(TARGET).run

$(eval $(call run_tmpl,$(TARGET)))

%.o: # скомпилировать, учитывая расположение -- app/main.c => app_main.o
	$(CC) -c $(subst _,/,$(basename $@)).cpp -o $(BUILD_DIR)$@

clean: 
	rm -f $(BUILD_DIR)*.o *.run

# $^ -- все зависимости
# $@ -- цель 
# $* -- шаблонная часть
# $< -- первый элемент в списке зависимостей
.PHONY: all clean

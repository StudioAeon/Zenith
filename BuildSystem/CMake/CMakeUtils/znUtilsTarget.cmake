function(zenith_enable_lto target)
	message(STATUS "Applying optimizations to: ${target}")

	if(IPO_SUPPORTED)
		set_target_properties(${target} PROPERTIES
				INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
				INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
		)
		message(STATUS "  -> IPO enabled for Release builds")
	endif()
endfunction()

function(zenith_copy_resources target)
	add_custom_command(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/Resources
			$<TARGET_FILE_DIR:${target}>/Resources
			COMMENT "Copying Resources to ${target}"
			VERBATIM
	)
endfunction()

function(zenith_add_sanitizers target)
	if(MSVC)
		target_compile_options(${target} PRIVATE /fsanitize=address)
	else()
		target_compile_options(${target} PRIVATE
				-fsanitize=address
				-fno-omit-frame-pointer
				-g
		)
		target_link_options(${target} PRIVATE -fsanitize=address)
	endif()
endfunction()
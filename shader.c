internal void
check_shader_errors(char *type, u32 object)
{
	int success;
	char info_log[512];
	if (strcmp("PROGRAM", type) != 0)
	{
		glGetShaderiv(object, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(object, 512, NULL, info_log);
			char temp[1024];
			if (strcmp("VERTEX", type) == 0)
			{
				wsprintfA(temp, "ERROR::SHADER::VERTEX::COMPILATION::FAILED - %s \n", info_log);
			}
			else
			{
				wsprintfA(temp, "ERROR::SHADER::FRAGMENT::COMPILATION::FAILED - %s \n", info_log);
			}
			printf("%s \n", temp);
		}
		else
		{
			if (strcmp("VERTEX", type) == 0)
			{
				OutputDebugStringA("Vertex Shader compiled successfully. \n");
			}
			else
			{
				OutputDebugStringA("Fragment Shader compiled successfully. \n");
			}
		}
	}
	else
	{
		glGetProgramiv(object, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(object, 512, NULL, info_log);
			char temp[1024];
			wsprintfA(temp, "ERROR::SHADER::PROGRAM::LINKING::FAILED - %s \n", info_log);
			OutputDebugStringA(temp);
		}
		else
		{
			OutputDebugStringA("Shader Program linked successfully. \n");
		}
	}
}
